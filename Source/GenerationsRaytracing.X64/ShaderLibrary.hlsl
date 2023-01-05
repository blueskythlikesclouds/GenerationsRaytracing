#include "ShaderFunctions.hlsli"

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

    float3 position = g_Position[index].xyz;
    float depth = g_Depth[index];
    float3 normal = normalize(g_Normal[index].xyz);
    float3 direction = GetCosHemisphereSample(normal);
    uint random = InitializeRandom(dimensions.x * index.y + index.x, g_CurrentFrame);

    float3 globalIllumination = TraceColor(position, direction, MAX_RECURSION_DEPTH - 3, random);

    int2 prevIndex = round(index + g_MotionVector[index]);

    float prevDepth = g_PrevDepth.Load(int3(prevIndex, 0));
    float3 prevNormal = g_PrevNormal.Load(int3(prevIndex, 0)).xyz;
    float4 prevGlobalIllumination = g_PrevGlobalIllumination.Load(int3(prevIndex, 0));

    float factor = abs(depth - prevDepth);
    factor = exp(factor / -0.01);
    factor *= saturate(dot(prevNormal, normal));
    factor *= prevGlobalIllumination.a;
    factor += 1.0;

    globalIllumination = lerp(prevGlobalIllumination.rgb, globalIllumination, 1.0 / factor);

    g_GlobalIllumination[index] = float4(globalIllumination, factor);
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
    uint2 dimensions = DispatchRaysDimensions().xy;

    uint4 shader = g_Shader[index];

    if ((shader.y & HAS_REFLECTION) == 0)
        return;

    float3 position = g_Position[index].xyz;
    float3 normal = normalize(g_Normal[index].xyz);
    float3 view = normalize(position - g_EyePosition.xyz);
    uint random = InitializeRandom(dimensions.x * index.y + index.x, g_CurrentFrame);

    float3 reflection = TraceReflection(position, normal, view, MAX_RECURSION_DEPTH - 2, random);

    g_Reflection[index] = float4(reflection, 1.0);
}

[shader("raygeneration")]
void RefractionRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    uint4 shader = g_Shader[index];

    if ((shader.y & HAS_REFRACTION) == 0)
        return;

    float3 position = g_Position[index].xyz;
    float3 normal = normalize(g_Normal[index].xyz);
    float3 view = normalize(position - g_EyePosition.xyz);
    uint random = InitializeRandom(dimensions.x * index.y + index.x, g_CurrentFrame);

    float3 refraction = TraceRefraction(position, normal, view, MAX_RECURSION_DEPTH - 2, random);

    g_Refraction[index] = float4(refraction, 1.0);
}

[shader("raygeneration")]
void CompositeRayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint4 shader = g_Shader[index];

    if (all(shader == 0))
        return;

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
    payload.Color = float3(179, 153, 128) / 255.0;
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