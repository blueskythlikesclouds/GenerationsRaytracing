#ifndef SHADER_LIBRARY_H_INCLUDED
#define SHADER_LIBRARY_H_INCLUDED

#include "ShaderFunctions.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    Payload payload = (Payload)0;
    payload.Random = InitializeRandom(index.x + index.y * dimensions.x, g_CurrentFrame);

    float2 ndc = (index - g_PixelJitter + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = g_CameraNearFarAspect.x;
    ray.TMax = g_CameraNearFarAspect.y;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload);

    float3 curPixelPosAndDepth = GetCurrentPixelPositionAndDepth(GetPosition(ray, payload));
    float3 prevPixelPosAndDepth = GetPreviousPixelPositionAndDepth(GetPreviousPosition(ray, payload));

    g_Color[index] = float4(payload.Color, 1.0);
    g_Depth[index] = payload.T == FLT_MAX ? 1.0 : curPixelPosAndDepth.z;
    g_MotionVector[index] = prevPixelPosAndDepth.xy - curPixelPosAndDepth.xy;
}

[shader("miss")]
void MissSkyBox(inout Payload payload : SV_RayPayload)
{
    RayDesc ray = (RayDesc)0;
    ray.Direction = WorldRayDirection();
    ray.TMax = Z_MAX;
    TraceRay(g_BVH, 0, 2, 0, 1, 1, ray, payload);

    payload.T = FLT_MAX;
}

[shader("miss")]
void MissEnvironmentColor(inout Payload payload : SV_RayPayload)
{
    payload.Color = 0.5;
    payload.T = FLT_MAX;
}

#endif