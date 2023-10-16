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
        shadingParams.DIReservoir = LoadDIReservoir(g_DIReservoirTexture[dispatchThreadId.xy]);
        shadingParams.GlobalIllumination = ComputeGI(gBufferData,
            LoadGIReservoir(g_GITexture, g_GIPositionTexture, g_GIReservoirTexture, dispatchThreadId.xy));

        float giLerpFactor = 0.0;

        int2 prevFrame = (float2) dispatchThreadId.xy - g_PixelJitter + 0.5 + g_MotionVectorsTexture[dispatchThreadId.xy];
        float3 prevNormal = normalize(g_PrevNormalTexture[prevFrame] * 2.0 - 1.0);

        if (g_CurrentFrame > 0 && dot(gBufferData.Normal, prevNormal) >= 0.9063)
        {
            float4 prevGlobalIllumination = g_PrevGIAccumulationTexture[prevFrame];
            giLerpFactor = min(30.0, prevGlobalIllumination.w + 1.0);
            shadingParams.GlobalIllumination = lerp(prevGlobalIllumination.rgb, shadingParams.GlobalIllumination, 1.0 / giLerpFactor);
        }

        g_GIAccumulationTexture[dispatchThreadId.xy] = float4(shadingParams.GlobalIllumination, giLerpFactor);

        float luminance = dot(shadingParams.GlobalIllumination, float3(0.299, 0.587, 0.114));
        shadingParams.GlobalIllumination = saturate((shadingParams.GlobalIllumination - luminance) * g_GI1Scale.w + luminance);
        shadingParams.GlobalIllumination *= g_GI0Scale.rgb;

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