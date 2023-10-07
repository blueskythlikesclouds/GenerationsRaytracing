#ifndef RAY_TRACING_H
#define RAY_TRACING_H
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

struct Payload
{
    float3 Color;
    float T;
    uint Depth;
};

float4 GetBlueNoise()
{
    Texture2D texture = ResourceDescriptorHeap[g_BlueNoiseTextureId];
    return texture.Load(int3((DispatchRaysIndex().xy + g_BlueNoiseOffset) % 1024, 0));
}

float3 GetCosineWeightedHemisphereSample(float2 random)
{
    float radius = sqrt(random.x);
    float angle = 2.0 * PI * random.y;

    return float3(radius * cos(angle), radius * sin(angle), sqrt(max(0.0, 1.0 - random.x)));
}

float3 GetPhongSample(float2 random, float specularPower)
{
    float cosTheta = pow(random.x, 1.0 / specularPower);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * random.y;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 TangentToWorld(float3 normal, float3 value)
{
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);
    return normalize(value.x * tangent + value.y * binormal + value.z * normal);
}

float TraceShadow(float3 position)
{
    float3 normal = -mrgGlobalLight_Direction.xyz;
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);

    float2 random = GetBlueNoise().xy;

    float angle = random.x * 2.0 * PI;
    float radius = sqrt(random.y) * 0.01;

    float3 direction;
    direction.x = cos(angle) * radius;
    direction.y = sin(angle) * radius;
    direction.z = sqrt(1.0 - saturate(dot(direction.xy, direction.xy)));

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = normalize(direction.x * tangent + direction.y * binormal + direction.z * normal);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    Payload payload1 = (Payload) 0;

    TraceRay(
            g_BVH,
            RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
            1,
            0,
            0,
            0,
            ray,
            payload1);

    return payload1.T >= 0.0 ? 0.0 : 1.0;
}

float3 TraceGlobalIllumination(uint depth, GBufferData gBufferData)
{
    if (depth > 1)
        return 0.0;

    float3 cosSample = GetCosineWeightedHemisphereSample(GetBlueNoise().xy);
    float3 rayDirection = TangentToWorld(gBufferData.Normal, cosSample);

    RayDesc ray;

    ray.Origin = gBufferData.Position;
    ray.Direction = rayDirection;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    Payload payload = (Payload) 0;
    payload.Depth = depth + 1;

    TraceRay(
            g_BVH,
            0,
            1,
            0,
            0,
            0,
            ray,
            payload);

    return payload.Color * (gBufferData.Diffuse + gBufferData.Falloff);
}

float3 TraceEnvironmentColor(uint depth, GBufferData gBufferData)
{
    if (depth != 0 || dot(gBufferData.Specular, gBufferData.Specular) * gBufferData.SpecularLevel < 0.001)
        return 0.0;

    float3 phongSample = GetPhongSample(GetBlueNoise().xy, gBufferData.SpecularPower);
    float3 halfwayDirection = TangentToWorld(gBufferData.Normal, phongSample);
    float3 reflectionDirection = normalize(reflect(WorldRayDirection(), halfwayDirection));

    RayDesc ray;

    ray.Origin = gBufferData.Position;
    ray.Direction = reflectionDirection;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    Payload payload = (Payload) 0;
    payload.Depth = depth + 1;

    TraceRay(
        g_BVH,
        0,
        1,
        0,
        0,
        0,
        ray,
        payload);

    float3 environmentColor = payload.Color;
    environmentColor *= gBufferData.Specular * gBufferData.SpecularLevel;

    float specular = dot(gBufferData.Normal, halfwayDirection);
    specular = pow(saturate(specular), gBufferData.SpecularPower);
    environmentColor *= specular;

    environmentColor *= saturate(dot(gBufferData.Normal, reflectionDirection));

    // TODO: I'm probably missing a PDF here somewhere...

    return environmentColor;
}

#endif