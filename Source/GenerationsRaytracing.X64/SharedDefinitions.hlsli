#pragma once

#define PI 3.14159265358979323846
#define FLT_MAX asfloat(0x7f7fffff)
#define INF asfloat(0x7f800000)

uint NextRandomUint(inout uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float NextRandomFloat(inout uint x)
{
    return 2.0 - asfloat((NextRandomUint(x) >> 9) | 0x3F800000);
}

float3 GetCosWeightedSample(float2 random)
{
    float cosTheta = sqrt(random.x);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * random.y;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float4 GetPowerCosWeightedSample(float2 random, float specularGloss)
{
    float cosTheta = pow(random.x, 1.0 / (specularGloss + 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2.0 * PI * random.y;
    float pdf = pow(cosTheta, specularGloss);
     
    return float4(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta, pdf);
}

float3 GetShadowSample(float2 random, float radius)
{
    radius *= sqrt(random.x);
    float angle = random.y * 2.0 * PI;

    float3 sample;
    sample.x = cos(angle) * radius;
    sample.y = sin(angle) * radius;
    sample.z = sqrt(1.0 - saturate(dot(sample.xy, sample.xy)));
    
    return sample;
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 NormalizeSafe(float3 value)
{
    float lengthSquared = dot(value, value);
    return select(lengthSquared > 0.0, value * rsqrt(lengthSquared), 0.0);
}

float3 TangentToWorld(float3 normal, float3 value)
{
    float3 binormal = GetPerpendicularVector(normal);
    float3 tangent = cross(binormal, normal);
    return NormalizeSafe(value.x * tangent + value.y * binormal + value.z * normal);
}

float2 ComputeNdcPosition(float3 position, float4x4 view, float4x4 projection)
{
    float4 projectedPosition = mul(float4(mul(float4(position, 0.0), view).xyz, 1.0), projection);
    return (projectedPosition.xy / projectedPosition.w * float2(0.5, -0.5) + 0.5);
}

float2 ComputePixelPosition(float3 position, float4x4 view, float4x4 projection)
{
    return ComputeNdcPosition(position, view, projection) * DispatchRaysDimensions().xy;
}

float ComputeDepth(float3 position, float4x4 projection)
{
    float4 projectedPosition = mul(float4(position, 1.0), projection);
    return projectedPosition.z / projectedPosition.w;
}

float ComputeDepth(float3 position, float4x4 view, float4x4 projection)
{
    return ComputeDepth(mul(float4(position, 0.0), view).xyz, projection);
}

float LinearizeDepth(float depth, float4x4 invProjection)
{
    float4 position = mul(float4(0, 0, depth, 1), invProjection);
    return position.z / position.w;
}

float3 ComputeFresnel(float3 specularFresnel, float cosTheta)
{
    return lerp(specularFresnel, 1.0, pow(saturate(1.0 - cosTheta), 5.0));
}

float ComputeWaterFresnel(float cosTheta)
{
    float specularFresnel = 1.0 - abs(cosTheta);
    specularFresnel *= specularFresnel;
    specularFresnel *= specularFresnel;
    return specularFresnel;
}

float3 DecodeNormalMap(float4 value)
{
    value.x *= value.w;

    float3 normalMap;
    normalMap.xy = value.xy * 2.0 - 1.0;
    normalMap.z = sqrt(abs(1.0 - dot(normalMap.xy, normalMap.xy)));
    return normalMap;
}

float3 BlendNormalMap(float3 left, float3 right)
{
    left += float3(0, 0, 1);
    right *= float3(-1, -1, 1);

    return left * dot(left, right) / left.z - right;
}

float ComputeFalloff(float3 normal, float3 falloffParam)
{
    return pow(1.0 - saturate(dot(normal, -WorldRayDirection())), falloffParam.z) * falloffParam.y + falloffParam.x;
}

float ConvertSpecularGlossToRoughness(float specularGloss)
{
    return 1.0 - pow(specularGloss, 0.2) * 0.25;
}