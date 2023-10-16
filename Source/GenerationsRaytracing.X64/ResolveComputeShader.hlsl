#include "GeometryShading.hlsli"
#include "RayTracing.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    float3 color = 0.0;
    float depth = 1.0;

    GBufferData gBufferData = LoadGBufferData(dispatchThreadId.xy);

    Reservoir<uint> diReservoir = (Reservoir<uint>)0;
    Reservoir<GISample> giReservoir = (Reservoir<GISample>)0;

    if (!(gBufferData.Flags & GBUFFER_FLAG_IS_SKY))
    {
        uint random = InitRand(g_CurrentFrame, dispatchThreadId.y * 1920 + dispatchThreadId.x);

        diReservoir = LoadDIReservoir(g_DIReservoirTexture[dispatchThreadId.xy]);

        giReservoir = LoadGIReservoir(
            g_GITexture, g_GIPositionTexture, g_GIReservoirTexture, dispatchThreadId.xy);

        float3 eyeDirection = normalize(g_EyePosition.xyz - gBufferData.Position);

        for (uint i = 0; i < 5; i++)
        {
            float radius = 8.0 * NextRand(random);
            float angle = 2.0 * PI * NextRand(random);

            int2 index = (float2) dispatchThreadId.xy + float2(cos(angle), sin(angle)) * radius;

            Reservoir<uint> spatialDIReservoir = LoadDIReservoir(g_DIReservoirTexture[index]);

            Reservoir<GISample> spatialGIReservoir = LoadGIReservoir(
                g_GITexture, g_GIPositionTexture, g_GIReservoirTexture, index);

            float3 normal = normalize(g_NormalTexture[index] * 2.0 - 1.0);

            if (dot(gBufferData.Normal, normal) >= 0.9063)
            {
                uint newSampleCount = diReservoir.SampleCount + spatialDIReservoir.SampleCount;

                UpdateReservoir(diReservoir, spatialDIReservoir.Sample,
                    ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[spatialDIReservoir.Sample]) * spatialDIReservoir.Weight * spatialDIReservoir.SampleCount, NextRand(random));

                diReservoir.SampleCount = newSampleCount;

                newSampleCount = giReservoir.SampleCount + spatialGIReservoir.SampleCount;
                UpdateReservoir(giReservoir, spatialGIReservoir.Sample,
                    ComputeGIReservoirWeight(gBufferData, spatialGIReservoir.Sample) * spatialGIReservoir.Weight * spatialGIReservoir.SampleCount, NextRand(random));

                giReservoir.SampleCount = newSampleCount;
            }
        }
        ComputeReservoirWeight(diReservoir, ComputeDIReservoirWeight(gBufferData, eyeDirection, g_LocalLights[diReservoir.Sample]));
        diReservoir.SampleCount = min(500, diReservoir.SampleCount);

        ComputeReservoirWeight(giReservoir, ComputeGIReservoirWeight(gBufferData, giReservoir.Sample));
        giReservoir.SampleCount = min(500, giReservoir.SampleCount);

        ShadingParams shadingParams;

        shadingParams.EyePosition = g_EyePosition.xyz;
        shadingParams.EyeDirection = eyeDirection;
        shadingParams.Shadow = g_ShadowTexture[dispatchThreadId.xy];
        shadingParams.DIReservoir = diReservoir;
        shadingParams.GlobalIllumination = ComputeGI(gBufferData, giReservoir);

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

    g_PrevDIReservoirTexture[dispatchThreadId.xy] = StoreDIReservoir(diReservoir);
    StoreGIReservoir(g_PrevGITexture, g_PrevGIPositionTexture, g_PrevGIReservoirTexture, dispatchThreadId.xy, giReservoir);
}