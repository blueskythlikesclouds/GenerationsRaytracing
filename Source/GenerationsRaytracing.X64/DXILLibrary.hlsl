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
    payload1.MotionVectors = payload.MotionVectors;
    payload1.T = payload.T;
    payload1.LayerIndex = 1;

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
    g_MotionVectors[DispatchRaysIndex().xy] = payload1.MotionVectors;
    g_LinearDepth[DispatchRaysIndex().xy] = payload1.T == INF ? g_CameraNearFarAspect.y : -mul(float4(ray.Origin + ray.Direction * payload1.T, 1.0), g_MtxView).z;    
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

    StoreGBufferData(uint3(DispatchRaysIndex().xy, 0), gBufferData);

    g_Depth[DispatchRaysIndex().xy] = 1.0;

    payload.MotionVectors =
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(gBufferData.Position, g_MtxView, g_MtxProjection);

    payload.T = INF;
}

[shader("miss")]
void PrimaryTransparentMiss(inout PrimaryTransparentRayPayload payload : SV_RayPayload)
{
    
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    uint randSeed = InitRandom(DispatchRaysIndex().xy);
    uint layerNum = g_LayerNum_SRV[DispatchRaysIndex().xy];
    
    for (uint i = 0; i < layerNum; i++)
    {
        uint3 dispatchRaysIndex = uint3(DispatchRaysIndex().xy, i);
        GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);

        if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT)) && i == 0 && g_LocalLightCount > 0)
        {
            float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
        
            // Generate initial candidates
            Reservoir reservoir = (Reservoir) 0;
            uint localLightCount = min(32, g_LocalLightCount);
    
            for (uint j = 0; j < localLightCount; j++)
            {
                uint sample = NextRandomUint(randSeed) % g_LocalLightCount;
                float weight = ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[sample]) * g_LocalLightCount;
                UpdateReservoir(reservoir, sample, weight, 1, NextRandomFloat(randSeed));
            }
    
            ComputeReservoirWeight(reservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Y]));
    
            // Temporal reuse
            if (g_CurrentFrame > 0)
            {
                int2 prevIndex = (float2) dispatchRaysIndex.xy - g_PixelJitter + 0.5 + g_MotionVectors_SRV[dispatchRaysIndex.xy];
    
                // TODO: Check for previous normal and depth
    
                Reservoir prevReservoir = LoadReservoir(g_PrevReservoir[prevIndex]);
                prevReservoir.M = min(reservoir.M * 20, prevReservoir.M);
            
                Reservoir temporalReservoir = (Reservoir) 0;
            
                UpdateReservoir(temporalReservoir, reservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                    g_LocalLights[reservoir.Y]) * reservoir.W * reservoir.M, reservoir.M, NextRandomFloat(randSeed));
            
                UpdateReservoir(temporalReservoir, prevReservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                    g_LocalLights[prevReservoir.Y]) * prevReservoir.W * prevReservoir.M, prevReservoir.M, NextRandomFloat(randSeed));
            
                ComputeReservoirWeight(temporalReservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[temporalReservoir.Y]));
            
                reservoir = temporalReservoir;
            }
    
            g_Reservoir[dispatchRaysIndex.xy] = StoreReservoir(reservoir);
        }
    }
}

[shader("raygeneration")]
void SecondaryRayGeneration()
{
    uint randSeed = InitRandom(DispatchRaysIndex().xy);
    uint layerNum = g_LayerNum_SRV[DispatchRaysIndex().xy];
    
    for (uint i = 0; i < layerNum; i++)
    {
        uint3 dispatchRaysIndex = uint3(DispatchRaysIndex().xy, i);
        GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);
        
        bool traceGlobalIllumination = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION));
        bool traceReflection = !(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION));
        
        if (traceGlobalIllumination || traceReflection)
        {
            float giProbability;
            if (traceGlobalIllumination ^ traceReflection)
                giProbability = traceGlobalIllumination ? 1.0 : 0.0;
            else
                giProbability = 1.0 - saturate(dot(ComputeReflection(gBufferData, 1.0), float3(0.299, 0.587, 0.114)));
        
            bool shouldTraceReflection = NextRandomFloat(randSeed) > giProbability;
        
            TracePathArgs args;
            args.Position = gBufferData.Position;
        
            if (shouldTraceReflection)
            {
                float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
    
                if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
                {
                    args.Direction = reflect(-eyeDirection, gBufferData.Normal);
                    args.MissShaderIndex = MISS_SECONDARY;
                    args.Throughput = 1.0;
                }
                else
                {
                    float4 sampleDirection = GetPowerCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed)), gBufferData.SpecularGloss);
                    float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
    
                    args.Direction = reflect(-eyeDirection, halfwayDirection);
                    args.MissShaderIndex = MISS_REFLECTION;
                    args.Throughput = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
                }
            
                args.Throughput /= 1.0 - giProbability;
                args.ApplyPower = false;
            }
            else
            {
                args.Direction = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(float2(NextRandomFloat(randSeed), NextRandomFloat(randSeed))));
                args.MissShaderIndex = MISS_GI;
                args.Throughput = 1.0 / giProbability;
                args.ApplyPower = true;
            }
        
            float4 radianceAndHitDistance = TracePath(args, randSeed);
        
            if (traceGlobalIllumination)
                g_GlobalIllumination[dispatchRaysIndex] = shouldTraceReflection ? 0.0 : radianceAndHitDistance;
        
            if (traceReflection)
                g_Reflection[dispatchRaysIndex] = shouldTraceReflection ? radianceAndHitDistance : 0.0;
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

[shader("miss")]
void ReflectionMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    float3 color = 0.0;

    if (g_SkyInRoughReflection)
    {
        if (g_UseEnvironmentColor)
        {
            color = WorldRayDirection().y > 0.0 ? g_SkyColor : g_GroundColor;
        }
        else if (g_UseSkyTexture)
        {
            TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
            color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
        }
    }

    payload.Color = color;
    payload.Diffuse = 0.0;
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
    MaterialData materialData = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, materialData.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    
    if (SampleMaterialTexture2D(materialData.Textures[0], vertex.TexCoords[0], 2).a < 0.5)
        IgnoreHit();
}