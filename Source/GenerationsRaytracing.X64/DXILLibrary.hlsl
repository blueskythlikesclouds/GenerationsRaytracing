#include "DXILLibrary.hlsli"

uint3 LoadDispatchRaysIndex(uint index)
{
    return uint3(index & 0x3FFF, (index >> 14) & 0x3FFF, (index >> 28) & 0xF);
}

uint StoreDispatchRaysIndex(uint2 index, uint layerIndex)
{
    return index.x | (index.y << 14) | (layerIndex << 28);
}

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
    payload1.LayerIndex = 1;
    payload1.RayGenerationFlags = payload.RayGenerationFlags;

    TraceRay(
        g_BVH,
        RAY_FLAG_CULL_FRONT_FACING_TRIANGLES | RAY_FLAG_FORCE_NON_OPAQUE | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        INSTANCE_MASK_TRANSPARENT,
        HIT_GROUP_PRIMARY_TRANSPARENT,
        HIT_GROUP_NUM,
        MISS_PRIMARY_TRANSPARENT,
        ray,
        payload1);

    [unroll]
    for (uint i = 0; i < RAY_GENERATION_NUM - 1; i++)
    {
        uint rayGenerationFlags = (payload1.RayGenerationFlags >> (i * GBUFFER_LAYER_NUM)) & ((1 << GBUFFER_LAYER_NUM) - 1);

        uint laneLayerNum = countbits(rayGenerationFlags);
        uint waveLayerNum = WaveActiveSum(laneLayerNum);

        uint baseDispatchRaysIndex;
        if (WaveIsFirstLane())
        {
            g_DispatchRaysDesc.InterlockedAdd(i * DISPATCH_RAYS_DESC_BYTE_SIZE +
                DISPATCH_RAYS_DESC_WIDTH_OFFSET, waveLayerNum, baseDispatchRaysIndex);
        }
        baseDispatchRaysIndex = WaveReadLaneFirst(baseDispatchRaysIndex);
        baseDispatchRaysIndex += i * DispatchRaysDimensions().x * DispatchRaysDimensions().y * GBUFFER_LAYER_NUM;

        [unroll]
        for (uint j = 0; j < GBUFFER_LAYER_NUM; j++)
        {
            bool layerValid = rayGenerationFlags & (1u << j);
            if (layerValid)
            {
                g_DispatchRaysIndex.Store((baseDispatchRaysIndex + WavePrefixSum(layerValid)) * sizeof(uint),
                    StoreDispatchRaysIndex(DispatchRaysIndex().xy, j));
            }

            baseDispatchRaysIndex += WaveActiveSum(layerValid);
        }
    }

    g_LayerNum[DispatchRaysIndex().xy] = payload1.LayerIndex;
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

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(gBufferData.Position, g_MtxView, g_MtxProjection);

    g_LinearDepth[DispatchRaysIndex().xy] = g_CameraNearFarAspect.y;

    payload.T = INF;
    payload.RayGenerationFlags = 0;
}

[shader("miss")]
void PrimaryTransparentMiss(inout PrimaryTransparentRayPayload payload : SV_RayPayload)
{
    
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    uint3 dispatchRaysIndex = LoadDispatchRaysIndex(g_DispatchRaysIndex.Load((
        g_InternalResolution.x * g_InternalResolution.y * GBUFFER_LAYER_NUM * (RAY_GENERATION_SHADOW - 1) +
        DispatchRaysIndex().x) * sizeof(uint)));

    GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);

    g_Shadow[dispatchRaysIndex] = TraceShadow(gBufferData.Position, 
        -mrgGlobalLight_Direction.xyz, GetBlueNoise(dispatchRaysIndex.xy).xy, RAY_FLAG_NONE, 0);

    if (dispatchRaysIndex.z == 0 && g_LocalLightCount > 0 && !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT))
    {
        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
    
        uint randSeed = InitRand(g_CurrentFrame,
            dispatchRaysIndex.y * g_InternalResolution.x + dispatchRaysIndex.x);
    
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
            int2 prevIndex = (float2) dispatchRaysIndex.xy - g_PixelJitter + 0.5 + g_MotionVectors_SRV[dispatchRaysIndex.xy];
    
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
    
        g_Reservoir[dispatchRaysIndex.xy] = StoreReservoir(reservoir);
    }
}

[shader("raygeneration")]
void GIRayGeneration()
{
    uint3 dispatchRaysIndex = LoadDispatchRaysIndex(g_DispatchRaysIndex.Load((
        g_InternalResolution.x * g_InternalResolution.y * GBUFFER_LAYER_NUM * (RAY_GENERATION_GI - 1) +
        DispatchRaysIndex().x) * sizeof(uint)));

    GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);
    float4 random = GetBlueNoise(dispatchRaysIndex.xy);

    g_GlobalIllumination[dispatchRaysIndex] = TracePath(
        gBufferData.Position, 
        TangentToWorld(gBufferData.Normal, GetCosWeightedSample(random.xy)),
        random,
        MISS_GI,
        dispatchRaysIndex.xy,
        false,
        1.0);
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
    uint3 dispatchRaysIndex = LoadDispatchRaysIndex(g_DispatchRaysIndex.Load((
        g_InternalResolution.x * g_InternalResolution.y * GBUFFER_LAYER_NUM * (RAY_GENERATION_REFLECTION - 1) +
        DispatchRaysIndex().x) * sizeof(uint)));

    GBufferData gBufferData = LoadGBufferData(dispatchRaysIndex);
    float4 random = GetBlueNoise(dispatchRaysIndex.xy);

    float3 rayDirection;
    float pdf;
    
    float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
    
    if (gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION))
    {
        rayDirection = reflect(-eyeDirection, gBufferData.Normal);
        pdf = 1.0;
    }
    else
    {
        float4 sampleDirection = GetPowerCosWeightedSample(random.xy, gBufferData.SpecularGloss);
        float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
    
        rayDirection = reflect(-eyeDirection, halfwayDirection);
        pdf = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
    }
    
    g_Reflection[dispatchRaysIndex] = TracePath(
        gBufferData.Position,
        rayDirection,
        random,
        MISS_SECONDARY,
        dispatchRaysIndex.xy,
        dispatchRaysIndex.z == 0,
        pdf);
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