#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "SharedDefinitions.hlsli"

struct [raypayload] PrimaryRayPayload
{
    float3 dDdx : read(anyhit, closesthit) : write(caller);
    float3 dDdy : read(anyhit, closesthit) : write(caller);
};

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

    PrimaryRayPayload payload = (PrimaryRayPayload) 0;

    ComputeRayDifferentials(
        rayDirection,
        mul(float4(1.0, 0.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[0][0],
        mul(float4(0.0, 1.0, 0.0, 0.0), g_MtxView).xyz / g_MtxProjection[1][1],
        1.0 / g_ViewportSize.zw,
        payload.dDdx,
        payload.dDdy);

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
    GBufferData gBufferData = CreateMissGBufferData(true, g_BackgroundColor);
    StoreGBufferData(uint3(DispatchRaysIndex().xy, GBUFFER_DATA_PRIMARY), gBufferData);

    g_Depth[DispatchRaysIndex().xy] = 1.0;

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(gBufferData.Position, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(gBufferData.Position, g_MtxView, g_MtxProjection);
}

[shader("closesthit")]
void PrimaryClosestHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, true);
    StoreGBufferData(uint3(DispatchRaysIndex().xy, GBUFFER_DATA_PRIMARY), CreateGBufferData(vertex, material));

    g_Depth[DispatchRaysIndex().xy] = ComputeDepth(vertex.Position, g_MtxView, g_MtxProjection);

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
}

float4 GetBlueNoise()
{
    Texture2D texture = ResourceDescriptorHeap[g_BlueNoiseTextureId];
    return texture.Load(int3((DispatchRaysIndex().xy + g_BlueNoiseOffset) % 1024, 0));
}

[shader("anyhit")]
void PrimaryAnyHit(inout PrimaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, true);
    GBufferData gBufferData = CreateGBufferData(vertex, material);
    float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : GetBlueNoise().x;

    if (gBufferData.Alpha < alphaThreshold)
        IgnoreHit();
}

[shader("raygeneration")]
void ReservoirRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, g_GBufferDataIndex));
    gBufferData.Specular = 0.0;
    gBufferData.TransColor = 0.0;

    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT)))
    {
        Reservoir reservoir = (Reservoir) 0;

        uint random = InitRand(g_CurrentFrame,
            DispatchRaysIndex().y * DispatchRaysDimensions().x + DispatchRaysIndex().x);

        uint localLightCount = min(32, g_LocalLightCount);

        for (uint i = 0; i < localLightCount; i++)
        {
            uint sample = min(floor(NextRand(random) * g_LocalLightCount), g_LocalLightCount - 1);
            float weight = ComputeReservoirWeight(gBufferData, 0.0, g_LocalLights[sample]) * g_LocalLightCount;
            UpdateReservoir(reservoir, sample, weight, NextRand(random));
        }

        ComputeReservoirWeight(reservoir, ComputeReservoirWeight(gBufferData, 0.0, g_LocalLights[reservoir.Y]));
        g_Reservoir[uint3(DispatchRaysIndex().xy, g_GBufferDataIndex)] = StoreReservoir(reservoir);
    }
}

struct [raypayload] SecondaryRayPayload
{
    uint Dummy : read() : write();
};

[shader("raygeneration")]
void SecondaryRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, GBUFFER_DATA_PRIMARY));

    float2 random = GetBlueNoise().xy;
    float3 rayDirection = 0.0;
    float tMax = INF;

    switch (g_GBufferDataIndex)
    {
        case GBUFFER_DATA_SECONDARY_GI:
            {
                if (gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
                    tMax = 0.0;

                rayDirection = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(random));
                break;
            }

        case GBUFFER_DATA_SECONDARY_REFLECTION:
            {
                if (gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION))
                    tMax = 0.0;

                float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);

                if (gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION))
                {
                    rayDirection = reflect(-eyeDirection, gBufferData.Normal);
                }
                else
                {
                    float4 sampleDirection = GetPowerCosWeightedSample(random, gBufferData.SpecularGloss);
                    float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);

                    rayDirection = reflect(-eyeDirection, halfwayDirection);

                    float pdf = pow(saturate(dot(gBufferData.Normal, halfwayDirection)), gBufferData.SpecularGloss) / (0.0001 + sampleDirection.w);
                    g_GBuffer2[uint3(DispatchRaysIndex().xy, 0)].w = pdf;

                }

                break;
            }

        case GBUFFER_DATA_SECONDARY_REFRACTION:
            {
                if (!(gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_OPACITY)))
                    tMax = 0.0;

                float3 eyeDirection = NormalizeSafe(g_EyePosition.xyz - gBufferData.Position);
                rayDirection = refract(-eyeDirection, gBufferData.Normal, 0.95);

                break;
            }
    }

    RayDesc ray;

    ray.Origin = gBufferData.Position;
    ray.Direction = rayDirection;
    ray.TMin = 0.0;
    ray.TMax = tMax;

    SecondaryRayPayload payload;

    TraceRay(
        g_BVH,
        0,
        1,
        1,
        0,
        1,
        ray,
        payload);
}

[shader("miss")]
void SecondaryMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    StoreGBufferData(uint3(DispatchRaysIndex().xy, g_GBufferDataIndex), 
        CreateMissGBufferData(g_GBufferDataIndex == GBUFFER_DATA_SECONDARY_REFLECTION || !g_UseEnvironmentColor));
}

[shader("closesthit")]
void SecondaryClosestHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, false);
    StoreGBufferData(uint3(DispatchRaysIndex().xy, g_GBufferDataIndex), CreateGBufferData(vertex, material));
}

[shader("anyhit")]
void SecondaryAnyHit(inout SecondaryRayPayload payload : SV_RayPayload, in BuiltInTriangleIntersectionAttributes attributes : SV_Attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];

    if (geometryDesc.Flags & GEOMETRY_FLAG_TRANSPARENT)
    {
        IgnoreHit();
    }
    else
    {
        Material material = g_Materials[geometryDesc.MaterialId];
        InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
        Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, false);

        if (SampleMaterialTexture2D(material.DiffuseTexture, vertex.TexCoords[0]).a < 0.5)
            IgnoreHit();
    }
}

[shader("raygeneration")]
void TertiaryRayGeneration()
{
    GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, g_GBufferDataIndex - GBUFFER_DATA_TERTIARY_GI + GBUFFER_DATA_SECONDARY_GI));

    RayDesc ray;

    ray.Origin = gBufferData.Position;
    ray.Direction = TangentToWorld(gBufferData.Normal, GetCosWeightedSample(GetBlueNoise().zw));
    ray.TMin = 0.0;
    ray.TMax = gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION) ? 0.0 : INF;

    SecondaryRayPayload payload;

    TraceRay(
        g_BVH,
        0,
        1,
        1,
        0,
        1,
        ray,
        payload);
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    g_Shadow[uint3(DispatchRaysIndex().xy, g_GBufferDataIndex)] = 0.0;

    float2 random = GetBlueNoise().xy;
    float radius = sqrt(random.x) * 0.01;
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = sqrt(1.0 - saturate(dot(sample.xy, sample.xy)));

    GBufferData gBufferData = LoadGBufferData(uint3(DispatchRaysIndex().xy, g_GBufferDataIndex));

    RayDesc ray;

    ray.Origin = gBufferData.Position;
    ray.Direction = TangentToWorld(-mrgGlobalLight_Direction.xyz, sample);
    ray.TMin = 0.0;
    ray.TMax = gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_SHADOW | GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT) ? 0.0 : INF;

    SecondaryRayPayload payload;

    TraceRay(
        g_BVH,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        1,
        1,
        0,
        2,
        ray,
        payload);
}

[shader("miss")]
void ShadowMiss(inout SecondaryRayPayload payload : SV_RayPayload)
{
    g_Shadow[uint3(DispatchRaysIndex().xy, g_GBufferDataIndex)] = 1.0;
}