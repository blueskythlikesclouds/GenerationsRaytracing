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

float3 GetGGXMicrofacetSample(float2 random, float roughness)
{
    float a2 = roughness * roughness;
    float cosTheta = sqrt(max(0.0, (1.0 - random.x) / ((a2 - 1.0) * random.x + 1)));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = 2.0 * PI * random.y;

    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
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

float ComputeFalloff(float3 normal, float3 falloffParam)
{
    return pow(1.0 - saturate(dot(normal, -WorldRayDirection())), falloffParam.z) * falloffParam.y + falloffParam.x;
}

float ConvertSpecularGlossToRoughness(float specularGloss)
{
    return 1.0 - pow(specularGloss, 0.2) * 0.25;
}

float3 FresnelSchlick(float3 F0, float cosTheta)
{
    float p = (-5.55473 * cosTheta - 6.98316) * cosTheta;
    return F0 + (1.0 - F0) * exp2(p);
}

float NdfGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * alphaSq - cosLh) * cosLh + 1;
    return alphaSq / (PI * denom * denom);
}

float VisSchlick(float roughness, float cosLo, float cosLi)
{
    float r = roughness + 1;
    float k = (r * r) / 8;
    float schlickV = cosLo * (1 - k) + k;
    float schlickL = cosLi * (1 - k) + k;
    return 0.25 / (schlickV * schlickL);
}

float2 ApproxEnvBRDF(float cosLo, float roughness)
{
    float4 c0 = float4(-1, -0.0275, -0.572, 0.022);
    float4 c1 = float4(1, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * cosLo)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}