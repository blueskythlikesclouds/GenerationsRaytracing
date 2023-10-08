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

float3 GetCosineWeightedHemisphere(float2 random)
{
    float cosTheta = sqrt(random.x);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * random.y;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float4 GetPowerCosineWeightedHemisphere(float2 random, float specularPower)
{
    float cosTheta = pow(random.x, 1.0 / (specularPower + 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * random.y;
    float pdf = (specularPower + 1.0) / (2.0 * PI);
     
    return float4(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta, pdf);
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
    float2 random = GetBlueNoise().xw;
    float radius = sqrt(random.x) * 0.01;
    float angle = random.y * 2.0 * PI;

    float3 direction;
    direction.x = cos(angle) * radius;
    direction.y = sin(angle) * radius;
    direction.z = sqrt(1.0 - saturate(dot(direction.xy, direction.xy)));

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(-mrgGlobalLight_Direction.xyz, direction);
    ray.TMin = 0.0;
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

    float4 random = GetBlueNoise();
    float3 sampleDirection = GetCosineWeightedHemisphere(depth == 0 ? random.xy : random.zw);
    float3 rayDirection = TangentToWorld(gBufferData.Normal, sampleDirection);

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
    float3 specularColor = gBufferData.Specular * gBufferData.SpecularLevel;

    if (depth > 0 || dot(specularColor, specularColor) == 0)
        return 0.0;

    float4 sampleDirection = GetPowerCosineWeightedHemisphere(GetBlueNoise().yz, gBufferData.SpecularPower);
    float3 halfwayDirection = TangentToWorld(gBufferData.Normal, sampleDirection.xyz);
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
    environmentColor *= gBufferData.Specular * gBufferData.SpecularLevel * 2.5;
    environmentColor /= sampleDirection.w;

    return environmentColor;
}

#endif