#include "GBufferData.hlsli"
#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"
#include "Reservoir.hlsli"

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    float2 ndc = (DispatchRaysIndex().xy - g_PixelJitter + 0.5) / DispatchRaysDimensions().xy * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = 0.0;
    ray.TMax = FLT_MAX;

    PrimaryRayPayload payload;

    TraceRay(
        g_BVH,
        0,
        1,
        0,
        0,
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
    gBufferData.SpecularPower = 1.0;
    gBufferData.Normal = -WorldRayDirection();
    gBufferData.Emission = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    StoreGBufferData(DispatchRaysIndex().xy, gBufferData);

    g_DepthTexture[DispatchRaysIndex().xy] = 1.0;

    g_MotionVectorsTexture[DispatchRaysIndex().xy] = 
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(gBufferData.Position, g_MtxView, g_MtxProjection);
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);
    StoreGBufferData(DispatchRaysIndex().xy, CreateGBufferData(vertex, material));

    g_DepthTexture[DispatchRaysIndex().xy] = ComputeDepth(vertex.Position, g_MtxView, g_MtxProjection);

    g_MotionVectorsTexture[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);
    GBufferData gBufferData = CreateGBufferData(vertex, material);
    float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : GetBlueNoise().x;

    if (gBufferData.Alpha < alphaThreshold)
        IgnoreHit();
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float shadow = 1.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_SHADOW)))
        shadow = TraceGlobalLightShadow(gBufferData.SafeSpawnPoint, -mrgGlobalLight_Direction.xyz);

    g_ShadowTexture[DispatchRaysIndex().xy] = shadow;
}

[shader("raygeneration")]
void ReservoirRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);
    float depth = g_DepthTexture[DispatchRaysIndex().xy];
    Reservoir<uint> reservoir = (Reservoir<uint>) 0;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT)))
    {
        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        uint random = InitRand(g_CurrentFrame,
            DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x);

        // Generate initial candidates
        uint localLightCount = min(32, g_LocalLightCount);

        for (uint i = 0; i < localLightCount; i++)
        {
            uint sample = min(floor(NextRand(random) * g_LocalLightCount), g_LocalLightCount - 1);
            float weight = ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[sample]) * g_LocalLightCount;
            UpdateReservoir(reservoir, sample, weight, NextRand(random));
        }

        ComputeReservoirWeight(reservoir, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Sample]));

        // Temporal reuse
        int2 temporalNeighbor = (float2) DispatchRaysIndex().xy - g_PixelJitter + 0.5 + g_MotionVectorsTexture[DispatchRaysIndex().xy];
        if (g_CurrentFrame > 0 && all(and(temporalNeighbor >= 0, temporalNeighbor < g_InternalResolution)))
        {
            float3 prevNormal = g_PrevNormalTexture[temporalNeighbor].xyz;

            if (abs(depth - g_DepthTexture[temporalNeighbor]) <= 0.1 && dot(prevNormal, gBufferData.Normal) >= 0.9063)
            {
                Reservoir<uint> prevReservoir = LoadDIReservoir(g_PrevDIReservoirTexture[temporalNeighbor]);
                prevReservoir.SampleCount = min(reservoir.SampleCount * 20, prevReservoir.SampleCount);

                Reservoir<uint> sumReservoir = (Reservoir<uint>) 0;

                UpdateReservoir(sumReservoir, reservoir.Sample, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Sample]) *
                    reservoir.Weight * reservoir.SampleCount, NextRand(random));

                UpdateReservoir(sumReservoir, prevReservoir.Sample, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[prevReservoir.Sample]) *
                    prevReservoir.Weight * prevReservoir.SampleCount, NextRand(random));

                sumReservoir.SampleCount = reservoir.SampleCount + prevReservoir.SampleCount;
                ComputeReservoirWeight(sumReservoir, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[sumReservoir.Sample]));

                reservoir = sumReservoir;
            }
        }
    }

    g_DIReservoirTexture[DispatchRaysIndex().xy] = StoreDIReservoir(reservoir);
}

[shader("raygeneration")]
void GIRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);
    float3 globalIllumination = 0.0;
    float3 rayDirection = 0.0;
    float hitDistance = 0.0;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION)))
    {
        rayDirection = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(GetBlueNoise().xy));

        RayDesc ray;

        ray.Origin = gBufferData.SafeSpawnPoint;
        ray.Direction = rayDirection;
        ray.TMin = 0.0;
        ray.TMax = FLT_MAX;

        SecondaryRayPayload payload;
        payload.Depth = 0;

        TraceRay(
            g_BVH,
            0,
            1,
            1,
            0,
            2,
            ray,
            payload);

        globalIllumination = payload.Color;
        hitDistance = payload.T;
    }

    g_GITexture[DispatchRaysIndex().xy] = globalIllumination;
    g_DiffuseRayDirectionHitDistanceTexture[DispatchRaysIndex().xy] = float4(rayDirection, hitDistance);
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 reflection = 0.0;
    float3 rayDirection = 0.0;
    float hitDistance = 0.0;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION)))
    {
        float pdf;

        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
        if (gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION))
        {
            rayDirection = reflect(-eyeDirection, gBufferData.Normal);
            pdf = 1.0;
        }
        else
        {
            float4 sampleDirection = GetPowerCosWeightedSample(GetBlueNoise().yz, gBufferData.SpecularPower);
            float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);

            rayDirection = reflect(-eyeDirection, halfwayDirection);
            pdf = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularPower) / (0.0001 + sampleDirection.w);
        }

        RayDesc ray;

        ray.Origin = gBufferData.SafeSpawnPoint;
        ray.Direction = rayDirection;
        ray.TMin = 0.0;
        ray.TMax = FLT_MAX;

        SecondaryRayPayload payload;
        payload.Depth = 0;

        TraceRay(
            g_BVH,
            0,
            1,
            1,
            0,
            3,
            ray,
            payload);

        reflection = payload.Color * pdf;
        hitDistance = payload.T;
    }

    g_ReflectionTexture[DispatchRaysIndex().xy] = reflection;
    g_SpecularRayDirectionHitDistanceTexture[DispatchRaysIndex().xy] = float4(rayDirection, hitDistance);
}

[shader("raygeneration")]
void RefractionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 refraction = 0.0;
    if (gBufferData.Flags & GBUFFER_FLAG_HAS_REFRACTION)
    {
        refraction = TraceRefraction(0, 
            gBufferData.SafeSpawnPoint, 
            gBufferData.Normal, 
            NormalizeSafe(g_EyePosition.xyz - gBufferData.Position));
    }
    g_RefractionTexture[DispatchRaysIndex().xy] = refraction;
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
    payload.Color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb;
    payload.T = Z_MAX;
}

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload : SV_RayPayload)
{
    payload.Miss = true;
}

[shader("miss")]
void GIMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    if (g_UseEnvironmentColor)
    {
        payload.Color = WorldRayDirection().y > 0.0 ? g_SkyColor : g_GroundColor;
        payload.T = Z_MAX;
    }
    else
    {
        TextureCube skyTexture = ResourceDescriptorHeap[g_SkyTextureId];
        payload.Color = skyTexture.SampleLevel(g_SamplerState, WorldRayDirection() * float3(1, 1, -1), 0).rgb * g_SkyPower;
        payload.T = Z_MAX;
    }
}

[shader("closesthit")]
void SecondaryClosestHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);

    GBufferData gBufferData = CreateGBufferData(vertex, material);
    gBufferData.Diffuse *= g_DiffusePower;

    float3 globalLighting = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        globalLighting = ComputeDirectLighting(gBufferData, -WorldRayDirection(), 
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb) * g_LightPower;
    }

    float3 eyeLighting = 0.0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_EYE_LIGHT))
        eyeLighting = ComputeEyeLighting(gBufferData, WorldRayOrigin(), -WorldRayDirection());

    int2 pixelPosition = round(ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection));
    float depth = ComputeDepth(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection);
    float3 normal = g_PrevNormalTexture[pixelPosition].xyz;

    float3 localLighting = 0.0;
    float3 globalIllumination = 0.0;
    bool traceGlobalIllumination = false;

    if (g_CurrentFrame > 0 && all(and(pixelPosition >= 0, pixelPosition < g_InternalResolution)) &&
        abs(g_PrevDepthTexture[pixelPosition] - depth) <= 0.05 && dot(gBufferData.Normal, normal) >= 0.9063)
    {
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT) && g_LocalLightCount > 0)
        {
            Reservoir<uint> diReservoir = LoadDIReservoir(g_PrevDIReservoirTexture[pixelPosition]);
            localLighting = ComputeLocalLighting(gBufferData, -WorldRayDirection(), g_LocalLights[diReservoir.Sample]) * diReservoir.Weight * g_LightPower;
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            globalIllumination = ComputeGI(gBufferData, g_GITexture[pixelPosition].rgb);
    }
    else
    {
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT) && g_LocalLightCount > 0)
        {
            uint sample = min(floor(GetBlueNoise().x * g_LocalLightCount), g_LocalLightCount - 1);
            localLighting = ComputeLocalLighting(gBufferData, -WorldRayDirection(), g_LocalLights[sample]) * g_LocalLightCount * g_LightPower;
        }

        traceGlobalIllumination = !(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION) && payload.Depth == 0;
    }
    
    float3 emissionLighting = gBufferData.Emission * g_EmissivePower;

    payload.Color = eyeLighting + localLighting + globalIllumination + emissionLighting;

    // Done at the end for reducing live state.
    if (traceGlobalIllumination)
        payload.Color += ComputeGI(gBufferData, TraceGI(payload.Depth + 1, gBufferData.SafeSpawnPoint, gBufferData.Normal));

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_SHADOW))
        globalLighting *= TraceGlobalLightShadow(vertex.SafeSpawnPoint, -mrgGlobalLight_Direction.xyz);

    payload.Color += globalLighting;
    payload.T = RayTCurrent();
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];

    [branch] if (geometryDesc.Flags & GEOMETRY_FLAG_TRANSPARENT)
    {
        IgnoreHit();
    }
    else
    {
        Material material = g_Materials[geometryDesc.MaterialId];
        InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
        Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes);

        if (SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords[0]).a < 0.5)
            IgnoreHit();
    }
}

