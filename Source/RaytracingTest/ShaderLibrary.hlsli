#include "ShaderDefinitions.hlsli"
#include "HSV.hlsli"

struct Payload
{
    float3 color;
    uint random;
    uint depth;
    float t;
};

struct Attributes
{
    float2 uv;
};

struct Material
{
    float4 float4Parameters[16];
    uint textureIndices[16];
};

RaytracingAccelerationStructure g_BVH : register(t0, space0);
RaytracingAccelerationStructure g_ShadowBVH : register(t1, space0);
RaytracingAccelerationStructure g_SkyBVH : register(t2, space0);

StructuredBuffer<Material> g_MaterialBuffer : register(t3, space0);
Buffer<uint4> g_MeshBuffer : register(t4, space0);

Buffer<float3> g_NormalBuffer : register(t5, space0);
Buffer<float4> g_TangentBuffer : register(t6, space0);
Buffer<float2> g_TexCoordBuffer : register(t7, space0);
Buffer<float4> g_ColorBuffer : register(t8, space0);

Buffer<uint> g_IndexBuffer : register(t9, space0);

Buffer<float4> g_LightBuffer : register(t10, space0);

SamplerState g_LinearRepeatSampler : register(s0, space0);

RWTexture2D<float4> g_Output : register(u0, space0);
RWTexture2D<float> g_OutputDepth : register(u1, space0);
RWTexture2D<float2> g_OutputMotionVector : register(u2, space0);

Texture2D<float4> g_BindlessTexture2D[] : register(t0, space1);
TextureCube<float4> g_BindlessTextureCube[] : register(t0, space2);

// http://intro-to-dxr.cwyman.org/

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
    if (payload.depth >= 4)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(GetCosHemisphereSample(payload.random, normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, INSTANCE_MASK_OPAQUE_OR_PUNCH | INSTANCE_MASK_TRANS_OR_SPECIAL, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float3 TraceReflection(inout Payload payload, float3 normal)
{
    if (payload.depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(reflect(WorldRayDirection(), normal));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, INSTANCE_MASK_OPAQUE_OR_PUNCH | INSTANCE_MASK_TRANS_OR_SPECIAL, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float3 TraceRefraction(inout Payload payload, float3 normal)
{
    if (payload.depth >= 2)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(refract(WorldRayDirection(), normal, 1.0 / 1.333));
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, INSTANCE_MASK_OPAQUE_OR_PUNCH | INSTANCE_MASK_TRANS_OR_SPECIAL, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float TraceShadow(inout uint random)
{
    float3 normal = -g_Globals.lightDirection;
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
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(direction.x * tangent + direction.y * binormal + direction.z * normal);
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload = (Payload)0;
    payload.depth = 0xFF;
    TraceRay(g_ShadowBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, INSTANCE_MASK_OPAQUE_OR_PUNCH, 0, 1, 0, ray, payload);

    return payload.t == FLT_MAX ? 1.0 : 0.0;
}

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    Payload payload = (Payload)0;
    payload.random = InitializeRandom(index.x + index.y * dimensions.x, g_Globals.currentFrame);

    float2 ndc = (index + g_Globals.pixelJitter + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_Globals.position;
    ray.Direction = normalize(mul(g_Globals.rotation, float4(ndc.x * g_Globals.tanFovy * g_Globals.aspectRatio, -ndc.y * g_Globals.tanFovy, -1.0, 0.0)).xyz);
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    TraceRay(g_BVH, 0, INSTANCE_MASK_OPAQUE_OR_PUNCH | INSTANCE_MASK_TRANS_OR_SPECIAL, 0, 1, 0, ray, payload);

    //g_Output[index] = lerp(g_Output[index], float4(payload.color, 1.0), 1.0 / g_Globals.currentFrame);
    g_Output[index] = float4(payload.color, 1.0);

    float3 position = ray.Origin + ray.Direction * min(payload.t, Z_MAX);

    float4 projectedPos = mul(g_Globals.projection, mul(g_Globals.view, float4(position, 1.0)));
    float2 pixelPos = (projectedPos.xy / projectedPos.w * float2(0.5, -0.5) + 0.5) * dimensions;

    float4 prevProjectedPos = mul(g_Globals.previousProjection, mul(g_Globals.previousView, float4(position, 1.0)));
    float2 prevPixelPos = (prevProjectedPos.xy / prevProjectedPos.w * float2(0.5, -0.5) + 0.5) * dimensions;

    g_OutputDepth[index] = projectedPos.z / projectedPos.w;
    g_OutputMotionVector[index] = prevPixelPos - pixelPos;
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    if (payload.depth != 0xFF)
    {
        RayDesc ray;
        ray.Origin = 0.0;
        ray.Direction = WorldRayDirection();
        ray.TMin = 0.001f;
        ray.TMax = Z_MAX;

        payload.depth = 0xFF;
        TraceRay(g_SkyBVH, 0, INSTANCE_MASK_SKY, 0, 1, 0, ray, payload);
    }

    payload.t = FLT_MAX;
}