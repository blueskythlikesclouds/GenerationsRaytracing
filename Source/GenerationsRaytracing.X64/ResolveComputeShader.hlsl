#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 color = 0.0;
    float depth = 1.0;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        ShadingParams shadingParams;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);
        shadingParams.Shadow = g_ShadowTexture[dispatchThreadId.xy];
        shadingParams.GlobalIllumination = g_GlobalIlluminationTexture[dispatchThreadId.xy];
        shadingParams.Reflection = g_ReflectionTexture[dispatchThreadId.xy];
        shadingParams.Refraction = g_RefractionTexture[dispatchThreadId.xy];

        if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
            color = ComputeWaterShading(gBufferData, shadingParams);
        else
            color = ComputeGeometryShading(gBufferData, shadingParams);

        float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;
        float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

        color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
        depth = ComputeDepth(gBufferData.Position, g_MtxView, g_MtxProjection);
    }
    else
    {
        color = g_EmissionTexture[dispatchThreadId.xy];
    }

    g_ColorTexture[dispatchThreadId.xy] = float4(color, 1.0);
    g_DepthTexture[dispatchThreadId.xy] = depth;
}