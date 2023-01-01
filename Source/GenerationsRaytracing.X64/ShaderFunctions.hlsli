#ifndef SHADER_FUNCTIONS_H_INCLUDED
#define SHADER_FUNCTIONS_H_INCLUDED

#include "ShaderDefinitions.hlsli"
#include "ShaderGlobals.hlsli"

float3 GetPosition(in RayDesc ray, in Payload payload)
{
    return ray.Origin + ray.Direction * min(payload.T, g_CameraNearFarAspect.y);
}

float3 GetPreviousPosition(in RayDesc ray, in Payload payload)
{
    return mul(float4(GetPosition(ray, payload), 1.0), g_InstanceBuffer[payload.InstanceIndex].Delta).xyz;
}

float3 GetPosition()
{
    return WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
}

float3 GetPixelPositionAndDepth(float3 position, float4x4 view, float4x4 projection)
{
    float4 screenCoords = mul(mul(float4(position, 1.0), view), projection);
    screenCoords /= screenCoords.w;

    screenCoords.xy = (screenCoords.xy * float2(0.5, -0.5) + 0.5) * DispatchRaysDimensions().xy;
    return screenCoords.xyz;
}

float3 GetCurrentPixelPositionAndDepth(float3 position)
{
    return GetPixelPositionAndDepth(position, g_MtxView, g_MtxProjection);
}

float3 GetPreviousPixelPositionAndDepth(float3 position)
{
    return GetPixelPositionAndDepth(position, g_MtxPrevView, g_MtxPrevProjection);
}

uint InitializeRandom(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;
    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

uint NextRandomUint(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return s;
}

float NextRandom(inout uint s)
{
    return float(NextRandomUint(s) & 0x00FFFFFF) / float(0x01000000);
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 GetCosHemisphereSample(inout uint randSeed, float3 hitNormal)
{
    float2 randomValue = float2(NextRandom(randSeed), NextRandom(randSeed));

    float3 bitangent = GetPerpendicularVector(hitNormal);
    float3 tangent = cross(bitangent, hitNormal);
    float r = sqrt(randomValue.x);
    float phi = 2.0f * 3.14159265f * randomValue.y;

    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNormal.xyz * sqrt(max(0.0, 1.0f - randomValue.x));
}

float3 TraceGlobalIllumination(inout Payload payload, float3 normal)
{
    if (payload.Depth >= 4)
        return 0;

    RayDesc ray;
    ray.Origin = GetPosition();
    ray.Direction = normalize(GetCosHemisphereSample(payload.Random, normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.Random = payload.Random;
    payload1.Depth = payload.Depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.Random = payload1.Random;
    return min(payload1.Color, 4.0);
}

float3 TraceReflection(inout Payload payload, float3 normal)
{
    if (payload.Depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = GetPosition();
    ray.Direction = normalize(reflect(WorldRayDirection(), normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.Random = payload.Random;
    payload1.Depth = payload.Depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.Random = payload1.Random;
    return min(payload1.Color, 4.0);
}

float3 TraceRefraction(inout Payload payload, float3 normal)
{
    if (payload.Depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = GetPosition();
    ray.Direction = normalize(refract(WorldRayDirection(), normal, 1.0 / 1.333));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.Random = payload.Random;
    payload1.Depth = payload.Depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.Random = payload1.Random;
    return min(payload1.Color, 4.0);
}

float TraceShadow(inout uint random)
{
    float3 normal = -mrgGlobalLight_Direction.xyz;
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);

    float x = NextRandom(random);
    float y = NextRandom(random);

    float angle = x * 6.28318530718;
    float radius = sqrt(y) * 0.01;

    float3 direction;
    direction.x = cos(angle) * radius;
    direction.y = sin(angle) * radius;
    direction.z = sqrt(1.0 - saturate(dot(direction.xy, direction.xy)));

    RayDesc ray;
    ray.Origin = GetPosition();
    ray.Direction = normalize(direction.x * tangent + direction.y * binormal + direction.z * normal);
    ray.TMin = 0.01f;
    ray.TMax = Z_MAX;

    Payload payload = (Payload)0;
    payload.Depth = 0xFF;
    TraceRay(g_BVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 1, 0, 1, 0, ray, payload);

    return payload.T == FLT_MAX ? 1.0 : 0.0;
}

#endif