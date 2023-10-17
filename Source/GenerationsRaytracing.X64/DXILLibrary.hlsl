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
    ray.TMax = g_CameraNearFarAspect.y;

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

    float shadow = 0.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT)))
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

        if (g_CurrentFrame > 0)
        {
            // Temporal reuse
            int2 temporalNeighbor = (float2) DispatchRaysIndex().xy - g_PixelJitter + 0.5 + g_MotionVectorsTexture[DispatchRaysIndex().xy];
            float3 prevNormal = NormalizeSafe(g_PrevNormalTexture[temporalNeighbor] * 2.0 - 1.0);

            if (abs(depth - g_DepthTexture[temporalNeighbor]) <= 0.1 && dot(prevNormal, gBufferData.Normal) >= 0.9063)
            {
                Reservoir<uint> prevReservoir = LoadDIReservoir(g_PrevDIReservoirTexture[temporalNeighbor]);
                prevReservoir.SampleCount = min(reservoir.SampleCount * 20, prevReservoir.SampleCount);

                Reservoir<uint> sumReservoir = (Reservoir<uint>) 0;

                UpdateReservoir(sumReservoir, reservoir.Sample, 
                    ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Sample]) * reservoir.Weight * reservoir.SampleCount, NextRand(random));

                UpdateReservoir(sumReservoir, prevReservoir.Sample, 
                    ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[prevReservoir.Sample]) * prevReservoir.Weight * prevReservoir.SampleCount, NextRand(random));

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
    float depth = g_DepthTexture[DispatchRaysIndex().xy];
    Reservoir<GISample> reservoir = (Reservoir<GISample>) 0;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION)))
    {
        uint random = InitRand(g_CurrentFrame,
            DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x);

        RayDesc ray;

        ray.Origin = gBufferData.SafeSpawnPoint;
        ray.Direction = TangentToWorld(gBufferData.Normal, GetUniformSample(float2(NextRand(random), NextRand(random))));
        ray.TMin = 0.0;
        ray.TMax = 10000.0;

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

        reservoir.Sample.Color = payload.Color;
        reservoir.Sample.Position = gBufferData.SafeSpawnPoint + payload.T * ray.Direction;
        reservoir.Sample.Normal = payload.Normal;
        reservoir.WeightSum = ComputeGIReservoirWeight(gBufferData, reservoir.Sample) * (2.0 * PI);
        reservoir.SampleCount = 1;

        if (g_CurrentFrame > 0)
        {
            // Temporal reuse
            int2 temporalNeighbor = (float2) DispatchRaysIndex().xy - g_PixelJitter + 0.5 + g_MotionVectorsTexture[DispatchRaysIndex().xy];
            float3 prevPosition = g_PrevPositionFlagsTexture[temporalNeighbor].xyz;
            float3 prevNormal = NormalizeSafe(g_PrevNormalTexture[temporalNeighbor] * 2.0 - 1.0);

            if (abs(depth - g_DepthTexture[temporalNeighbor]) <= 0.1 && dot(prevNormal, gBufferData.Normal) >= 0.9063)
            {
                Reservoir<GISample> prevReservoir = LoadGIReservoir(g_PrevGITexture,
                    g_PrevGIPositionTexture, g_PrevGINormalTexture, temporalNeighbor);

                float jacobian = ComputeJacobian(gBufferData.Position, prevPosition, prevReservoir.Sample);
                if (jacobian >= 1.0 / 10.0 && jacobian <= 10.0)
                {
                    prevReservoir.SampleCount = min(8, prevReservoir.SampleCount);
                    uint newSampleCount = reservoir.SampleCount + prevReservoir.SampleCount;

                    UpdateReservoir(reservoir, prevReservoir.Sample, ComputeGIReservoirWeight(gBufferData, prevReservoir.Sample) * 
                        prevReservoir.Weight * prevReservoir.SampleCount * clamp(jacobian, 1.0 / 3.0, 3.0), NextRand(random));

                    reservoir.SampleCount = newSampleCount;
                }
            }
        }

        ComputeReservoirWeight(reservoir, ComputeGIReservoirWeight(gBufferData, reservoir.Sample));
    }

    StoreGIReservoir(g_GITexture, g_GIPositionTexture, g_GINormalTexture, DispatchRaysIndex().xy, reservoir);
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(DispatchRaysIndex().xy);

    float3 reflection = 0.0;
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION)))
    {
        float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

        if (gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION)
        {
            reflection = TraceReflection(0, 
                gBufferData.SafeSpawnPoint, 
                gBufferData.Normal,
                eyeDirection);
        }
        else
        {
            reflection = TraceReflection(0,
                gBufferData.SafeSpawnPoint,
                gBufferData.Normal,
                gBufferData.SpecularPower,
                eyeDirection);
        }
    }
    g_ReflectionTexture[DispatchRaysIndex().xy] = reflection;
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
    payload.T = 10000.0;
    payload.Normal = -WorldRayDirection();
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
        payload.Color = lerp(g_GroundColor, g_SkyColor, WorldRayDirection().y * 0.5 + 0.5);
        payload.T = 10000.0;
        payload.Normal = -WorldRayDirection();
    }
    else
    {
        SecondaryMiss(payload);
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

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT))
    {
        payload.Color = ComputeDirectLighting(gBufferData, -WorldRayDirection(), 
            -mrgGlobalLight_Direction.xyz, mrgGlobalLight_Diffuse.rgb, mrgGlobalLight_Specular.rgb) * g_LightPower;

        payload.Color *= TraceGlobalLightShadow(vertex.Position, -mrgGlobalLight_Direction.xyz);
    }
    else
    {
        payload.Color = 0.0;
    }

    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_EYE_LIGHT))
        payload.Color += ComputeEyeLighting(gBufferData, WorldRayOrigin(), -WorldRayDirection());

    float2 pixelPosition = ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection);
    float depth = ComputeDepth(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection);
    float3 normal = NormalizeSafe(g_PrevNormalTexture[pixelPosition] * 2.0 - 1.0);

    if (g_CurrentFrame > 0 && abs(g_PrevDepthTexture[pixelPosition] - depth) <= 0.1 && dot(gBufferData.Normal, normal) >= 0.9063)
    {
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT) && g_LocalLightCount > 0)
        {
            Reservoir<uint> diReservoir = LoadDIReservoir(g_PrevDIReservoirTexture[pixelPosition]);
            payload.Color += ComputeLocalLighting(gBufferData, -WorldRayDirection(), g_LocalLights[diReservoir.Sample]) * diReservoir.Weight * g_LightPower;
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            payload.Color += ComputeGI(gBufferData, g_PrevGIAccumulationTexture[pixelPosition].rgb);
    }
    else
    {
        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT) && g_LocalLightCount > 0)
        {
            uint sample = min(floor(GetBlueNoise().x * g_LocalLightCount), g_LocalLightCount - 1);
            payload.Color += ComputeLocalLighting(gBufferData, -WorldRayDirection(), g_LocalLights[sample]) * g_LocalLightCount * g_LightPower;
        }

        if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION) && payload.Depth == 0)
            payload.Color += ComputeGI(gBufferData, TraceGI(payload.Depth + 1, gBufferData.SafeSpawnPoint, gBufferData.Normal));
    }

    payload.Color += gBufferData.Emission * g_EmissivePower;
    payload.T = RayTCurrent();
    payload.Normal = gBufferData.Normal;
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

