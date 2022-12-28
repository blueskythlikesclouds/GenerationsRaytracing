#ifndef SHADER_LIBRARY_H_INCLUDED
#define SHADER_LIBRARY_H_INCLUDED

#include "ShaderFunctions.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    float3 previousColor = g_Texture[index].rgb;
    float previousFactor = g_Texture[index].w;

    Payload payload = (Payload)0;
    payload.Random = InitializeRandom(index.x + index.y * dimensions.x, (uint)previousFactor);

    float2 offset = 0.0;

    if (previousFactor > 0.0)
    {
        float u1 = 2.0 * NextRandom(payload.Random);
        float u2 = 2.0 * NextRandom(payload.Random);

        offset.x = u1 < 1 ? sqrt(u1) - 1.0 : 1.0 - sqrt(2.0 - u1);
        offset.y = u2 < 1 ? sqrt(u2) - 1.0 : 1.0 - sqrt(2.0 - u2);
    }

    float2 ndc = (index + offset + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = g_CameraNearFarAspect.x;
    ray.TMax = g_CameraNearFarAspect.y;

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload);

    if (previousFactor > 0)
    {
        float factor = previousFactor + 1.0;
        g_Texture[index] = float4(lerp(previousColor, payload.Color, 1.0 / factor), factor);
    }
    else
    {
        g_Texture[index] = float4(payload.Color, 1.0);
    }

    g_Depth[index] = payload.T == FLT_MAX ? 1.0 : GetCurrentPixelPositionAndDepth(GetPosition(ray, payload)).z;
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    if (payload.Depth != 0xFF)
    {
        RayDesc ray;
        ray.Origin = 0.0;
        ray.Direction = WorldRayDirection();
        ray.TMin = 0.001f;
        ray.TMax = Z_MAX;

        payload.Depth = 0xFF;
        TraceRay(g_BVH, 0, 2, 0, 1, 0, ray, payload);
    }

    payload.T = FLT_MAX;
}

#endif