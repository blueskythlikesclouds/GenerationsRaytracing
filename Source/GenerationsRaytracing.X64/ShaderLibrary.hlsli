#ifndef SHADER_LIBRARY_H_INCLUDED
#define SHADER_LIBRARY_H_INCLUDED

#include "ShaderFunctions.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 index = DispatchRaysIndex().xy;
    uint2 dimensions = DispatchRaysDimensions().xy;

    float2 ndc = (index + 0.5) / dimensions * 2.0 - 1.0;

    RayDesc ray;
    ray.Origin = g_EyePosition.xyz;
    ray.Direction = normalize(mul(g_MtxView, float4(ndc.x / g_MtxProjection[0][0], -ndc.y / g_MtxProjection[1][1], -1.0, 0.0)).xyz);
    ray.TMin = 0.001f;
    ray.TMax = Z_MAX;

    Payload payload = (Payload)0;
#if 0
    payload.Random = InitializeRandom(index.x + index.y * dimensions.x, (uint)(g_TimeParam.y * 60.0f));
#else
    payload.Random = InitializeRandom(index.x, index.y);
#endif

    TraceRay(g_BVH, 0, 1, 0, 1, 0, ray, payload);

    float3 pixelPosAndDepth = GetCurrentPixelPositionAndDepth(GetPosition(ray, payload));

#if 0
    float4 previous = g_Texture[index];
    float factor = min(64, previous.w + 1.0);

    if (abs(g_Depth[index] - pixelPosAndDepth.z) > 0.01)
    {
        previous = 0.0;
        factor = 1.0;
    }

    g_Texture[index] = float4(lerp(previous.rgb, payload.Color, 1.0 / factor), factor);
#else
    g_Texture[index] = float4(payload.Color, 1.0);
#endif
    g_Depth[index] = pixelPosAndDepth.z;
}

[shader("miss")]
void Miss(inout Payload payload : SV_RayPayload)
{
    payload.Color = float3(106.0f / 255.0f, 113.0f / 255.0f, 179.0f / 255.0f);
    payload.T = FLT_MAX;
}

#endif