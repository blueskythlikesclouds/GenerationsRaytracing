#include "DXILLibrary.hlsli"

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    float3 rayDirection = ComputePrimaryRayDirection(DispatchRaysIndex().xy, DispatchRaysDimensions().xy);

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(rayDirection);
    ray.TMin = 0.0;
    ray.TMax = INF;

    float3 dDdx;
    float3 dDdy;

    ComputeRayDifferentials(
        rayDirection,
        mul(float4(1.0, 0.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[0][0],
        mul(float4(0.0, 1.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[1][1],
        1.0 / g_ViewportSize.zw,
        dDdx,
        dDdy);

    PrimaryRayPayload payload;
    payload.dDdx = dDdx;
    payload.dDdy = dDdy;

    TraceRay(
        g_BVH,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        INSTANCE_MASK_OPAQUE,
        HIT_GROUP_PRIMARY,
        HIT_GROUP_NUM,
        MISS_PRIMARY,
        ray,
        payload);

    ray.TMax = payload.T;

    PrimaryTransparentRayPayload payload1;
    payload1.dDdx = dDdx;
    payload1.dDdy = dDdy;
    payload1.LayerIndex = payload.T != INF ? 1 : 0;

    TraceRay(
        g_BVH,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES | RAY_FLAG_FORCE_NON_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        INSTANCE_MASK_TRANSPARENT,
        HIT_GROUP_PRIMARY_TRANSPARENT,
        HIT_GROUP_NUM,
        MISS_PRIMARY_TRANSPARENT,
        ray,
        payload1);

    g_LayerNum[DispatchRaysIndex().xy] = payload1.LayerIndex;
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload : SV_RayPayload)
{
    g_Depth[DispatchRaysIndex().xy] = 1.0;

    float3 position = WorldRayOrigin() + WorldRayDirection() * g_CameraNearFarAspect.y;

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(position, g_MtxView, g_MtxProjection);

    g_LinearDepth[DispatchRaysIndex().xy] = g_CameraNearFarAspect.y;

    payload.T = INF;
}

[shader("miss")]
void PrimaryTransparentMiss(inout PrimaryTransparentRayPayload payload : SV_RayPayload)
{
    
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    uint layerNum = g_LayerNum[DispatchRaysIndex().xy];
    for (uint i = 0; i < layerNum; i++)
    {
        GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, i));

        bool resolveReservoir = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT));
        if (i == 0 && g_LocalLightCount > 0 && resolveReservoir)
        {
            float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

            uint randSeed = InitRand(g_CurrentFrame,
                DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x);

            // Generate initial candidates
            Reservoir reservoir = (Reservoir) 0;
            uint localLightCount = min(32, g_LocalLightCount);

            [loop]
            for (uint j = 0; j < localLightCount; j++)
            {
                uint sample = min(floor(NextRand(randSeed) * g_LocalLightCount), g_LocalLightCount - 1);
                float weight = ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[sample]) * g_LocalLightCount;
                UpdateReservoir(reservoir, sample, weight, 1, NextRand(randSeed));
            }

            ComputeReservoirWeight(reservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Y]));

            // Temporal reuse
            if (g_CurrentFrame > 0)
            {
                int2 prevIndex = (float2) DispatchRaysIndex().xy - g_PixelJitter + 0.5 + g_MotionVectors[DispatchRaysIndex().xy];

                // TODO: Check for previous normal and depth

                Reservoir prevReservoir = LoadReservoir(g_PrevReservoir[prevIndex]);
                prevReservoir.M = min(reservoir.M * 20, prevReservoir.M);
                
                Reservoir temporalReservoir = (Reservoir) 0;
                
                UpdateReservoir(temporalReservoir, reservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                    g_LocalLights[reservoir.Y]) * reservoir.W * reservoir.M, reservoir.M, NextRand(randSeed));
                
                UpdateReservoir(temporalReservoir, prevReservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                    g_LocalLights[prevReservoir.Y]) * prevReservoir.W * prevReservoir.M, prevReservoir.M, NextRand(randSeed));
                
                ComputeReservoirWeight(temporalReservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[temporalReservoir.Y]));
                
                reservoir = temporalReservoir;
            }

            g_Reservoir[DispatchRaysIndex().xy] = StoreReservoir(reservoir);
        }

        bool traceShadow = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_SHADOW));
        if (traceShadow)
            g_Shadow[uint3(DispatchRaysIndex().xy, i)] = TraceShadow(gBufferData.Position, -mrgGlobalLight_Direction.xyz, GetBlueNoise().xy, RAY_FLAG_NONE, 0);
    }
}

[shader("raygeneration")]
void GIRayGeneration()
{
    uint layerNum = g_LayerNum[DispatchRaysIndex().xy];
    for (uint i = 0; i < layerNum; i++)
    {
        GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, i));
        bool traceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));

        if (traceGlobalIllumination)
        {
            g_GlobalIllumination[uint3(DispatchRaysIndex().xy, i)] = TracePath(
                gBufferData.Position, 
                TangentToWorld(gBufferData.Normal, GetCosWeightedSample(GetBlueNoise().xy)),
                MISS_GI,
                false);
        }
    }
}

[shader("miss")]
void GIMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_UseEnvironmentColor)
    {
        color = WorldRayDirection().y > 0.0 ? g_SkyColor : g_GroundColor;
    }
    else if (g_UseSkyTexture)
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb * g_SkyPower;
    }

    payload.Color = color;
    payload.Diffuse = 0.0;
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    uint layerNum = g_LayerNum[DispatchRaysIndex().xy];
    for (uint i = 0; i < layerNum; i++)
    {
        GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, i));
        bool traceReflection = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION));

        if (traceReflection)
        {
            float3 rayDirection;
            float pdf;

            float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

            [flatten]
            if (gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION))
            {
                rayDirection = reflect(-eyeDirection, gBufferData.Normal);
                pdf = 1.0;
            }
            else
            {
                float4 sampleDirection = GetPowerCosWeightedSample(GetBlueNoise().xy, gBufferData.SpecularGloss);
                float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);

                rayDirection = reflect(-eyeDirection, halfwayDirection);
                pdf = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
            }

            g_Reflection[uint3(DispatchRaysIndex().xy, i)] = pdf * TracePath(
                gBufferData.Position,
                rayDirection,
                MISS_SECONDARY,
                true);
        }
    }
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_UseSkyTexture)
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    }

    payload.Color = color;
    payload.Diffuse = 0.0;
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    
    if (SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords[0], 2).a < 0.5)
        IgnoreHit();
}