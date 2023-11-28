#include "GBufferData.hlsli"
#include "GeometryShading.hlsli"
#include "Reservoir.hlsli"

struct [raypayload] PrimaryRayPayload
{
    float3 dDdx : read(closesthit) : write(caller);
    float Random : read(anyhit) : write(caller);
    float3 dDdy : read(closesthit) : write(caller);
};

void PrimaryAnyHit(uint shaderType,
    inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);
    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);

    float random = payload.Random;
    float alphaThreshold = geometryDesc.Flags & GEOMETRY_FLAG_PUNCH_THROUGH ? 0.5 : random;

    if (gBufferData.Alpha < alphaThreshold)
        IgnoreHit();
}

void PrimaryClosestHit(uint vertexFlags, uint shaderType,
    inout PrimaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, payload.dDdx, payload.dDdy, vertexFlags);
    StoreGBufferData(DispatchRaysIndex().xy, CreateGBufferData(vertex, material, shaderType));

    g_Depth[DispatchRaysIndex().xy] = ComputeDepth(vertex.Position, g_MtxView, g_MtxProjection);

    g_MotionVectors[DispatchRaysIndex().xy] =
        ComputePixelPosition(vertex.PrevPosition, g_MtxPrevView, g_MtxPrevProjection) -
        ComputePixelPosition(vertex.Position, g_MtxView, g_MtxProjection);
}

struct [raypayload] ShadowRayPayload
{
    bool Miss : read(caller) : write(caller, miss);
};

float TraceShadow(float3 position, float3 direction, float2 random, float tMax = INF)
{
    float radius = sqrt(random.x) * 0.01;
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = sqrt(1.0 - saturate(dot(sample.xy, sample.xy)));

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(direction, sample);
    ray.TMin = 0.0;
    ray.TMax = tMax;

    ShadowRayPayload payload = (ShadowRayPayload) 0;

    TraceRay(
        g_BVH,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        1,
        1,
        3,
        1,
        ray,
        payload);

    return payload.Miss ? 1.0 : 0.0;
}

struct [raypayload] SecondaryRayPayload
{
    float3 Color : read(caller) : write(closesthit, miss);
    float NormalX : read(closesthit, caller) : write(closesthit, caller);
    float3 Diffuse : read(caller) : write(closesthit, miss);
    float NormalY : read(caller) : write(closesthit);
    float3 Position : read(caller) : write(closesthit);
    float NormalZ : read(caller) : write(closesthit);
};

float3 TracePath(float3 position, float3 direction, float3 throughput, uint missShaderIndex)
{
    float3 radiance = 0.0;
    float4 random = GetBlueNoise();

    [unroll]
    for (uint i = 0; i < 2; i++)
    {
        RayDesc ray;
        ray.Origin = position;
        ray.Direction = direction;
        ray.TMin = 0.0;
        ray.TMax = any(throughput != 0.0) ? INF : 0.0;

        SecondaryRayPayload payload;
        payload.NormalX = random.x;

        TraceRay(
            g_BVH,
            i > 0 ? RAY_FLAG_CULL_NON_OPAQUE : RAY_FLAG_NONE,
            1,
            2,
            3,
            i == 0 ? missShaderIndex : 2,
            ray,
            payload);

        float3 color = payload.Color;
        float3 normal = float3(payload.NormalX, payload.NormalY, payload.NormalZ);

        if (WaveActiveAnyTrue(any(payload.Diffuse != 0)))
        {
            color += payload.Diffuse * mrgGlobalLight_Diffuse.rgb * g_LightPower * saturate(dot(-mrgGlobalLight_Direction.xyz, normal)) *
               TraceShadow(payload.Position, -mrgGlobalLight_Direction.xyz, i == 0 ? random.xy : random.zw, any(payload.Diffuse != 0) ? INF : 0.0);
        }

        radiance += throughput * color;

        if (WaveActiveAllTrue(all(payload.Diffuse == 0)))
            break;

        position = payload.Position;
        direction = TangentToWorld(normal, GetCosWeightedSample(i == 0 ? random.xz : random.yw));
        throughput *= payload.Diffuse;
    }

    return radiance;
}

void SecondaryClosestHit(uint shaderType,
    inout SecondaryRayPayload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    GeometryDesc geometryDesc = g_GeometryDescs[InstanceID() + GeometryIndex()];
    Material material = g_Materials[geometryDesc.MaterialId];
    InstanceDesc instanceDesc = g_InstanceDescs[InstanceIndex()];
    Vertex vertex = LoadVertex(geometryDesc, material.TexCoordOffsets, instanceDesc, attributes, 0.0, 0.0, VERTEX_FLAG_NONE);

    GBufferData gBufferData = CreateGBufferData(vertex, material, shaderType);
    gBufferData.Diffuse *= g_DiffusePower;
    gBufferData.Emission *= g_EmissivePower;

    float3 color = gBufferData.Emission;

    [flatten]
    if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_LOCAL_LIGHT) && g_LocalLightCount > 0)
    {
        uint sample = min(floor(payload.NormalX * g_LocalLightCount), g_LocalLightCount - 1);
        color += ComputeLocalLighting(gBufferData, -WorldRayDirection(), g_LocalLights[sample]) * g_LocalLightCount * g_LightPower;
    }

    payload.Color = color;
    payload.Diffuse = gBufferData.Diffuse;
    payload.Position = gBufferData.Position;
    payload.NormalX = gBufferData.Normal.x;
    payload.NormalY = gBufferData.Normal.y;
    payload.NormalZ = gBufferData.Normal.z;
}

