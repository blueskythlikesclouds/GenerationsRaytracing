#include "DXILLibrary.hlsli"

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy - g_PixelJitter + 0.5) / DispatchRaysDimensions().xy * float2(2.0, -2.0) + float2(-1.0, 1.0);
    float3 rayDirection = mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz;

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
    payload.Random = GetBlueNoise().x;

    TraceRay(
        g_BVH,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES,
        1,
        0,
        2,
        0,
        ray,
        payload);
}

[shader("miss")]
void PrimaryMiss(inout PrimaryRayPayload payload : SV_RayPayload)
{
    TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];

    GBufferData gBufferData = (GBufferData) 0;
    gBufferData.Position = WorldRayOrigin() + WorldRayDirection() * g_CameraNearFarAspect.y;
    gBufferData.Flags = GBUFFER_FLAG_IS_SKY;
    gBufferData.SpecularGloss = 1.0;
    gBufferData.Normal = -WorldRayDirection();

    if (g_UseSkyTexture)
        gBufferData.Emission = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    else
        gBufferData.Emission = g_BackgroundColor;

    StoreGBufferData(DispatchRaysIndex().xy, gBufferData);

    g_Depth[DispatchRaysIndex().xy] = 1.0;

    g_MotionVectors[DispatchRaysIndex().xy] = 
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(gBufferData.Position, g_MtxView, g_MtxProjection);

    g_LinearDepth[DispatchRaysIndex().xy] = g_CameraNearFarAspect.y;
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    bool resolveReservoir = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT));
    if (g_LocalLightCount > 0 && resolveReservoir)
    {
        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        uint randSeed = InitRand(g_CurrentFrame,
            DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x);

        // Generate initial candidates
        Reservoir reservoir = (Reservoir) 0;
        uint localLightCount = min(32, g_LocalLightCount);

        [loop]
        for (uint i = 0; i < localLightCount; i++)
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
        g_Shadow[DispatchRaysIndex().xy] = TraceShadow(gBufferData.Position, -mrgGlobalLight_Direction.xyz, GetBlueNoise().xy, RAY_FLAG_NONE, 0);
}

[shader("raygeneration")]
void GIRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);
    bool traceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));

    if (traceGlobalIllumination)
    {
        g_GlobalIllumination[DispatchRaysIndex().xy] = TracePath(
            gBufferData.Position, 
            TangentToWorld(gBufferData.Normal, GetCosWeightedSample(GetBlueNoise().xy)),
            1,
            false);
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
    else if (g_UseSkyTexture && RayTCurrent() != 0)
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
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);
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

        g_Reflection[DispatchRaysIndex().xy] = pdf * TracePath(
            gBufferData.Position,
            rayDirection,
            2,
            true);
    }
}

[shader("raygeneration")]
void RefractionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    bool traceRefraction = gBufferData.Flags &
        (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_IS_ADDITIVE);

    if (traceRefraction)
    {
        float3 eyeDirection = NormalizeSafe(gBufferData.Position - g_EyePosition.xyz);

        float3 rayDirection = gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_OPACITY) ? 
            refract(eyeDirection, gBufferData.Normal, 0.95) : eyeDirection;
        
        g_Refraction[DispatchRaysIndex().xy] = TracePath(
            gBufferData.Position, 
            rayDirection,
            2,
            false);
    }
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_UseSkyTexture && RayTCurrent() != 0)
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

    [branch]
    if (geometryDesc.Flags & GEOMETRY_FLAG_TRANSPARENT)
    {
        IgnoreHit();
    }
    else
    {
        Material material = g_Materials[geometryDesc.MaterialId];
        InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
        Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);

        if (SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords[0], 2).a < 0.5)
            IgnoreHit();
    }
}