#define FLT_MAX asfloat(0x7f7fffff)

struct Payload
{
    float4 color : COLOR;
    uint random : RANDOM;
    uint depth : DEPTH;
    bool miss : MISS;
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
    float4 parameters[16];
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

RWTexture2D<float4> g_Output : register(u0, space0);

SamplerState g_Sampler : register(s0, space0);

Texture2D<float4> g_BindlessTexture[] : register(t0, space1);

// http://intro-to-dxr.cwyman.org/

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
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
void nextRandUint(inout uint s)
{
    s = (1664525u * s + 1013904223u);
}
// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    nextRandUint(s);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}
// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
    // Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));
    // Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;
    // Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(max(0.0, 1.0f - randVal.x));
}

float4 TraceGlobalIllumination(inout Payload payload, float3 position, float3 normal)
{
    if (payload.depth >= 4)
        return 0;

    RayDesc ray;
    ray.Origin = position;
    ray.Direction = getCosHemisphereSample(payload.random, normal);
    ray.TMin = 0.01f;
    ray.TMax = FLT_MAX;

    Payload payload1 = (Payload)0;
    payload1.random = payload.random;
    payload1.depth = payload.depth + 1;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload1);

    payload.random = payload1.random;

    return payload1.color;
}

float TraceShadow(float3 position)
{
    RayDesc ray;
    ray.Origin = position;
    ray.Direction = -g_Globals.lightDirection;
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
    payload.random = initRand(index.x + index.y * dimensions.x, g_Globals.sampleCount);

    float u1 = 2.0f * nextRand(payload.random);
    float u2 = 2.0f * nextRand(payload.random);
    float dx = u1 < 1 ? sqrt(u1) - 1.0 : 1.0 - sqrt(2.0 - u1);
    float dy = u2 < 1 ? sqrt(u2) - 1.0 : 1.0 - sqrt(2.0 - u2);

    float2 ndc = (index + float2(dx, dy) + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_Globals.position;
    ray.Direction = normalize(mul(g_Globals.rotation, float4(ndc.x * g_Globals.tanFovy * g_Globals.aspectRatio, -ndc.y * g_Globals.tanFovy, -1.0, 0.0)).xyz);
    ray.TMin = 0.001f;
    ray.TMax = FLT_MAX;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload);

    g_Output[index] = (g_Output[index] * (g_Globals.sampleCount - 1) + payload.color) / g_Globals.sampleCount;
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

[shader("anyhit")]
void AnyHit(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)
{
    uint4 mesh = g_MeshBuffer[InstanceID() + GeometryIndex()];

    if (mesh.w != 0)
    {
        uint3 indices;
        indices.x = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 0];
        indices.y = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 1];
        indices.z = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 2];

        float3 uv = float3(1.0 - attributes.uv.x - attributes.uv.y, attributes.uv.x, attributes.uv.y);

        float2 texCoord =
            g_TexCoordBuffer[indices.x] * uv.x +
            g_TexCoordBuffer[indices.y] * uv.y +
            g_TexCoordBuffer[indices.z] * uv.z;

    	float4 vertexColor =
            g_ColorBuffer[indices.x] * uv.x +
            g_ColorBuffer[indices.y] * uv.y +
            g_ColorBuffer[indices.z] * uv.z;

        float4 color = g_BindlessTexture[NonUniformResourceIndex(g_MaterialBuffer[mesh.z].textureIndices[0])].SampleLevel(g_Sampler, texCoord, 0) * vertexColor;
        if (mesh.w == 2)
        {
            if (color.a < 0.5)
                IgnoreHit();
        }
        else
        {
            if (color.a < nextRand(payload.random))
                IgnoreHit();
        }
    }
}

[shader("closesthit")]
void ClosestHit(inout Payload payload : SV_RayPayload, Attributes attributes : SV_IntersectionAttributes)
{
    const float3 lightDirection = normalize(float3(0.5, 1, 0));

    float3 position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 viewDirection = normalize(WorldRayOrigin() - position);

    RayDesc shadowRay;
    shadowRay.Origin = position;
    shadowRay.Direction = lightDirection;
    shadowRay.TMin = 0.01f;
    shadowRay.TMax = FLT_MAX;

    Payload shadowPayload = (Payload)0;
    TraceRay(g_BVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 1, 0, 1, 0, shadowRay, shadowPayload);

    uint4 mesh = g_MeshBuffer[InstanceID() + GeometryIndex()];

    uint3 indices;
    indices.x = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 0];
    indices.y = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 1];
    indices.z = mesh.x + g_IndexBuffer[mesh.y + PrimitiveIndex() * 3 + 2];

    float3 uv = float3(1.0 - attributes.uv.x - attributes.uv.y, attributes.uv.x, attributes.uv.y);

    float3 normal = 
        g_NormalBuffer[indices.x] * uv.x + 
        g_NormalBuffer[indices.y] * uv.y + 
        g_NormalBuffer[indices.z] * uv.z;

    normal = normalize(mul(ObjectToWorld3x4(), float4(normal, 0.0))).xyz;

    float3 reflectionDirection = reflect(-viewDirection, normal);

    RayDesc reflectionRay;
    reflectionRay.Origin = position;
    reflectionRay.Direction = reflectionDirection;
    reflectionRay.TMin = 0.01f;
    reflectionRay.TMax = FLT_MAX;

    Payload reflectionPayload = (Payload)0;
    reflectionPayload.random = payload.random;
    reflectionPayload.depth = payload.depth + 1;

    if (payload.depth < 2)
		TraceRay(g_BVH, 0, 1, 0, 1, 0, reflectionRay, reflectionPayload);

    payload.random = reflectionPayload.random;

    RayDesc giRay;
    giRay.Origin = position;
    giRay.Direction = getCosHemisphereSample(payload.random, normal);
    giRay.TMin = 0.01f;
    giRay.TMax = FLT_MAX;

    Payload giPayload = (Payload)0;
    giPayload.random = payload.random;
    giPayload.depth = payload.depth + 1;

    if (payload.depth < 4)
        TraceRay(g_BVH, 0, 1, 0, 1, 0, giRay, giPayload);

    payload.random = giPayload.random;

    float2 texCoord =
    	g_TexCoordBuffer[indices.x] * uv.x +
        g_TexCoordBuffer[indices.y] * uv.y +
        g_TexCoordBuffer[indices.z] * uv.z;

    float4 color = g_BindlessTexture[NonUniformResourceIndex(g_MaterialBuffer[mesh.z].textureIndices[0])].SampleLevel(g_Sampler, texCoord, 0);

    payload.color.rgb = (saturate(dot(normal, lightDirection)) * 1.25 * (shadowPayload.miss ? 1.0 : 0.0) + giPayload.color.rgb) * color.rgb + reflectionPayload.color.rgb * pow(saturate(1.0 - dot(normal, viewDirection)), 5.0);
    payload.color.a = 1.0;
}