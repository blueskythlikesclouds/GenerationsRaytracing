#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (any(dispatchThreadId.xy >= g_InternalResolution))
        return;

    float3 composite = g_ColorBeforeTransparency_SRV[dispatchThreadId.xy];

    uint layerNum = min(GBUFFER_LAYER_NUM, g_LayerNum_SRV[dispatchThreadId.xy]);
    if (layerNum > 1)
    {
        --layerNum;

        float4 color[GBUFFER_LAYER_NUM - 1];
        float distance[GBUFFER_LAYER_NUM - 1];
        bool additive[GBUFFER_LAYER_NUM - 1];

        for (uint i = 0; i < layerNum; i++)
        {
            GBufferData gBufferData = LoadGBufferData(uint3(dispatchThreadId.xy, i + 1));

            ShadingParams shadingParams = (ShadingParams) 0;
            shadingParams.EyePosition = g_EyePosition.xyz;
            shadingParams.EyeDirection = g_EyePosition.xyz - gBufferData.Position;

            distance[i] = length(shadingParams.EyeDirection);
            shadingParams.EyeDirection /= distance[i];
            additive[i] = (gBufferData.Flags & GBUFFER_FLAG_IS_ADDITIVE) != 0;

            if (!(gBufferData.Flags & (GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_SHADOW)))
                shadingParams.Shadow = g_Shadow_SRV[uint3(dispatchThreadId.xy, i + 1)];
            else
                shadingParams.Shadow = 1.0;

            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
                shadingParams.GlobalIllumination = g_GlobalIllumination_SRV[uint3(dispatchThreadId.xy, i + 1)];

            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
                shadingParams.Reflection = g_Reflection_SRV[uint3(dispatchThreadId.xy, i + 1)];

            float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;

            if (gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_OPACITY))
            {
                float2 texCoord = (dispatchThreadId.xy + 0.5) / g_InternalResolution;
                texCoord = texCoord * 2.0 - 1.0;
                texCoord += gBufferData.Normal.xz * 0.05;
                texCoord = texCoord * 0.5 + 0.5;
                shadingParams.Refraction = g_ColorBeforeTransparency_SRV.SampleLevel(g_SamplerState, texCoord, 0);

                if (ComputeDepth(gBufferData.Position, g_MtxView, g_MtxProjection) >= g_Depth_SRV[texCoord * g_InternalResolution])
                    shadingParams.Refraction = composite;

                float3 viewPositionToCompare = mul(
                    float4(LoadGBufferData(uint3(dispatchThreadId.xy, 0)).Position, 1.0), g_MtxView).xyz;

                gBufferData.Alpha *= saturate((viewPosition.z - viewPositionToCompare.z) * 0.5 / 8.0);
            }

            if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                color[i].rgb = ComputeWaterShading(gBufferData, shadingParams);
            else
                color[i].rgb = ComputeGeometryShading(gBufferData, shadingParams);

            float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

            if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
                color[i].rgb = color[i].rgb * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;

            color[i].a = gBufferData.Alpha;
        }

        for (uint j = 0; j < layerNum - 1; j++)
        {
            for (uint k = 0; k < layerNum - j - 1; k++)
            {
                if (distance[k] < distance[k + 1])
                {
                    float4 tempColor = color[k + 1];
                    float tempDistance = distance[k + 1];

                    color[k + 1] = color[k];
                    distance[k + 1] = distance[k];
                    color[k] = tempColor;
                    distance[k] = tempDistance;
                }
            }
        }

        for (uint i = 0; i < layerNum; i++)
        {
            if (!additive[i])
                composite *= 1.0 - color[i].a;

            composite += color[i].rgb * color[i].a;
        }
    }

    g_Color[dispatchThreadId.xy] = float4(composite, 1.0);
}