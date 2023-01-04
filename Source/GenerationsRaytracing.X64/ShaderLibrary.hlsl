#include "ShaderFunctions.hlsli"

Texture2D<float4> g_BlueNoise : register(t4);
Texture2D<float> g_PrevDepth : register(t5);
Texture2D<float4> g_PrevNormal : register(t6);
Texture2D<float4> g_PrevGlobalIllumination : register(t7);

float3 GetCosHemisphereSampleBlueNoise(float3 hitNormal)
{
    float2 index = DispatchRaysIndex().xy + float2(17, 31) * g_CurrentFrame;
    float2 randomValue = g_BlueNoise.Load(int3(index, 0) % 512).xy;

    float3 bitangent = GetPerpendicularVector(hitNormal);
    float3 tangent = cross(bitangent, hitNormal);
    float r = sqrt(randomValue.x);
    float phi = 2.0f * 3.14159265f * randomValue.y;

    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNormal.xyz * sqrt(max(0.0, 1.0f - randomValue.x));
}

[shader("raygeneration")]
void PrimaryRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    float2 ndc = (index - g_PixelJitter + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = g_CameraNearFarAspect.x;
    ray.TMax = g_CameraNearFarAspect.y;

    Payload payload = (Payload)0;
    payload.Random = InitializeRandom(dimensions.x * index.y + index.x, g_CurrentFrame);

    TraceRay(
        g_BVH, 
        0, 
        INSTANCE_MASK_DEFAULT, 
        CLOSEST_HIT_PRIMARY, 
        CLOSEST_HIT_NUM, 
        MISS_PRIMARY_SKY, 
        ray, 
        payload);
}

[shader("raygeneration")]
void GlobalIlluminationRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    uint4 shader = g_Shader[index];
    if ((shader.y & HAS_GLOBAL_ILLUMINATION) == 0)
        return;

    float3 globalIllumination = TraceColor(
        g_Position[index].xyz,
        GetCosHemisphereSampleBlueNoise(g_Normal[index].xyz),
        MAX_RECURSION_DEPTH - 3);

    int2 prevIndex = round(index + g_MotionVector[index]);
    float prevDepth = g_PrevDepth.Load(int3(prevIndex, 0));
    float3 prevNormal = g_PrevNormal.Load(int3(prevIndex, 0)).xyz;
    float4 prevGlobalIllumination = g_PrevGlobalIllumination.Load(int3(prevIndex, 0));

    float factor = exp(abs(g_Depth[index] - prevDepth) / -0.01) * saturate(dot(prevNormal, g_Normal[index].xyz)) * prevGlobalIllumination.a + 1.0;

    g_GlobalIllumination[index] = float4(lerp(prevGlobalIllumination.rgb, globalIllumination, 1.0 / factor), factor);
}

[shader("raygeneration")]
void ShadowRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    uint4 shader = g_Shader[index];
    if ((shader.y & HAS_GLOBAL_ILLUMINATION) == 0)
        return;

    uint random = InitializeRandom(dimensions.x * index.y + index.x, g_CurrentFrame);
    g_Shadow[index] = TraceShadow(g_Position[index].xyz, random);
}

[shader("raygeneration")]
void ReflectionRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint4 shader = g_Shader[index];

    if ((shader.y & HAS_REFLECTION) == 0)
        return;

    g_Reflection[index] = float4(TraceReflection(
        g_Position[index].xyz, 
        g_Normal[index].xyz, 
        normalize(g_Position[index].xyz - g_EyePosition.xyz), 
        MAX_RECURSION_DEPTH - 2), 1.0);
}

[shader("raygeneration")]
void RefractionRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint4 shader = g_Shader[index];

    if ((shader.y & HAS_REFRACTION) == 0)
        return;

    g_Refraction[index] = float4(TraceRefraction(
        g_Position[index].xyz, 
        g_Normal[index].xyz, 
        normalize(g_Position[index].xyz - g_EyePosition.xyz), 
        MAX_RECURSION_DEPTH - 2), 1.0);
}

[shader("raygeneration")]
void CompositeRayGeneration()
{
    uint4 shader = g_Shader[DispatchRaysIndex().xy];

    CallableParams callableParams = (CallableParams)0;
    callableParams.MaterialIndex = shader.z;

    CallShader(shader.x, callableParams);
}

[shader("miss")]
void MissPrimary(inout Payload payload : SV_RayPayload)
{
}

[shader("miss")]
void MissPrimarySky(inout Payload payload : SV_RayPayload)
{
    RayDesc ray = (RayDesc)0;
    ray.Direction = WorldRayDirection();
    ray.TMax = Z_MAX;

    TraceRay(
        g_BVH,
        0,
        INSTANCE_MASK_SKY,
        CLOSEST_HIT_PRIMARY,
        CLOSEST_HIT_NUM,
        MISS_PRIMARY,
        ray,
        payload);
}

[shader("miss")]
void MissSecondary(inout Payload payload : SV_RayPayload)
{
    payload.T = FLT_MAX;
}

[shader("miss")]
void MissSecondarySky(inout Payload payload : SV_RayPayload)
{
    RayDesc ray = (RayDesc)0;
    ray.Direction = WorldRayDirection();
    ray.TMax = Z_MAX;

    TraceRay(
        g_BVH, 
        0, 
        INSTANCE_MASK_SKY, 
        CLOSEST_HIT_SECONDARY, 
        CLOSEST_HIT_NUM, 
        MISS_SECONDARY, 
        ray, 
        payload);

    payload.T = FLT_MAX;
}