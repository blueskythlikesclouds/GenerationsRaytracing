#define FLT_MAX asfloat(0x7f7fffff)

struct Payload
{
    float4 color;
    uint random;
    uint depth;
    bool miss;
};

struct Attributes
{
    float2 uv;
};

struct cbGlobals
{
    float3 position;
    float tanFovy;
    float4x4 rotation;
    float aspectRatio;
    uint sampleCount;
    float3 lightDirection;
    float3 lightColor;
};

ConstantBuffer<cbGlobals> g_Globals : register(b0, space0);

struct Material
{
    float4 float4Parameters[16];
    uint textureIndices[16];
};

RaytracingAccelerationStructure g_BVH : register(t0, space0);

StructuredBuffer<Material> g_MaterialBuffer : register(t1, space0);
Buffer<uint4> g_MeshBuffer : register(t2, space0);

Buffer<float3> g_NormalBuffer : register(t3, space0);
Buffer<float4> g_TangentBuffer : register(t4, space0);
Buffer<float2> g_TexCoordBuffer : register(t5, space0);
Buffer<float4> g_ColorBuffer : register(t6, space0);

Buffer<uint> g_IndexBuffer : register(t7, space0);

SamplerState g_DefaultSampler : register(s0, space0);
SamplerComparisonState g_ComparisonSampler : register(s1, space0);

RWTexture2D<float4> g_Output : register(u0, space0);

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

void NextRandomUint(inout uint s)
{
    s = (1664525u * s + 1013904223u);
}

float NextRandom(inout uint s)
{
    NextRandomUint(s);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

float3 GetCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
    float2 randVal = float2(NextRandom(randSeed), NextRandom(randSeed));

	float3 bitangent = GetPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

float4 TraceGlobalIllumination(inout Payload payload, float3 normal)
{
    if (payload.depth >= 4)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = GetCosHemisphereSample(payload.random, normal);
    ray.TMin = 0.01f;
    ray.TMax = FLT_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float4 TraceReflection(inout Payload payload, float3 normal)
{
    if (payload.depth >= 1)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = normalize(reflect(normalize(ray.Origin - WorldRayOrigin()), normal));
    ray.TMin = 0.01f;
    ray.TMax = FLT_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;
    return payload1.color;
}

float4 TraceRefraction(inout Payload payload)
{
    if (payload.depth >= 1)
        return 0;

    RayDesc ray;
    ray.Origin = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    ray.Direction = WorldRayDirection();
    ray.TMin = 0.01f;
    ray.TMax = FLT_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

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
    ray.TMin = 0.01f;
    ray.TMax = FLT_MAX;

    Payload payload = (Payload)0;
    payload.depth = 0xFF;
    TraceRay(g_BVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 1, 0, 1, 0, ray, payload);

    return payload.miss ? 1.0 : 0.0;
}

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    Payload payload = (Payload)0;
    payload.random = InitializeRandom(index.x + index.y * dimensions.x, g_Globals.sampleCount);

    float u1 = 2.0f * NextRandom(payload.random);
    float u2 = 2.0f * NextRandom(payload.random);
    float dx = u1 < 1 ? sqrt(u1) - 1.0 : 1.0 - sqrt(2.0 - u1);
    float dy = u2 < 1 ? sqrt(u2) - 1.0 : 1.0 - sqrt(2.0 - u2);

    float2 ndc = (index + float2(dx, dy) + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_Globals.position;
    ray.Direction = normalize(mul(g_Globals.rotation, float4(ndc.x * g_Globals.tanFovy * g_Globals.aspectRatio, -ndc.y * g_Globals.tanFovy, -1.0, 0.0)).xyz);
    ray.TMin = 0.001f;
    ray.TMax = FLT_MAX;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload);

    g_Output[index] = lerp(g_Output[index], payload.color, 1.0 / g_Globals.sampleCount);
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    if (payload.depth != 0xFF)
    {
        RayDesc ray;
        ray.Origin = 0.0;
        ray.Direction = WorldRayDirection();
        ray.TMin = 0.01f;
        ray.TMax = FLT_MAX;

        payload.depth = 0xFF;
        TraceRay(g_BVH, 0, 2, 0, 1, 0, ray, payload);
    }
    else
    {
        payload.color = 0.0;
    }

    payload.miss = true;
}