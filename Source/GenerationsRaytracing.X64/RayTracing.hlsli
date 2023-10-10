#ifndef RAY_TRACING_H
#define RAY_TRACING_H
#include "RootSignature.hlsli"
#include "SharedDefinitions.hlsli"

struct [raypayload] PrimaryRayPayload
{
    uint dummy : read() : write();
};

struct [raypayload] SecondaryRayPayload
{
    float3 Color : read(caller)     : write(closesthit, miss);
    float T      : read(caller)     : write(closesthit, miss);
    uint Depth   : read(closesthit) : write(caller);
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
    float pdf = pow(cosTheta, specularPower);
     
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

float TraceHardShadow(float3 position, float3 direction)
{
    RayDesc ray;

    ray.Origin = position;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    SecondaryRayPayload payload = (SecondaryRayPayload) 0;

    TraceRay(
        g_BVH,
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
        1,
        1,
        0,
        1,
        ray,
        payload);

    return payload.T >= 0.0 ? 0.0 : 1.0;
}

float TraceSoftShadow(float3 position, float3 direction)
{
    float2 random = GetBlueNoise().xw;
    float radius = sqrt(random.x) * 0.01;
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = sqrt(1.0 - saturate(dot(sample.xy, sample.xy)));

    return TraceHardShadow(position, TangentToWorld(direction, sample));
}

float3 TraceGlobalIllumination(uint depth, float3 position, float3 normal)
{
    if (depth > 1)
        return 0.0;

    float4 random = GetBlueNoise();
    float3 sampleDirection = GetCosineWeightedHemisphere(depth == 0 ? random.xy : random.zw);

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = TangentToWorld(normal, sampleDirection);
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    SecondaryRayPayload payload = (SecondaryRayPayload) 0;
    payload.Depth = depth + 1;

    TraceRay(
        g_BVH,
        0,
        1,
        1,
        0,
        1,
        ray,
        payload);

    return payload.Color;
}

float3 TraceReflection(
    uint depth,
    float3 position,
    float3 direction)
{
    if (depth > 0)
        return 0.0;

    RayDesc ray;

    ray.Origin = position;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;

    SecondaryRayPayload payload = (SecondaryRayPayload) 0;
    payload.Depth = depth + 1;

    TraceRay(
        g_BVH,
        0,
        1,
        1,
        0,
        1,
        ray,
        payload);

    return payload.Color;
}

float3 TraceReflection(
    uint depth,
    float3 position,
    float3 normal,
    float specularPower,
    float3 eyeDirection)
{
    float4 sampleDirection = GetPowerCosineWeightedHemisphere(GetBlueNoise().yz, specularPower);
    float3 halfwayDirection = TangentToWorld(normal, sampleDirection.xyz);
    float3 reflectionDirection = normalize(reflect(-eyeDirection, halfwayDirection));

    return TraceReflection(depth, position, reflectionDirection) * 
        pow(saturate(dot(normal, halfwayDirection)), specularPower) / max(0.000001, sampleDirection.w * PI);
}

#endif