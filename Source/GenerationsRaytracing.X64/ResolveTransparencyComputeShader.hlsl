#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"
#include "ThreadGroupTilingX.hlsl"

struct Layer
{
    float4 Color;
    float3 DiffuseAlbedo;
    float3 SpecularAlbedo;
    float Distance;
    bool Additive;
};

[numthreads(8, 8, 1)]
void main(uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    uint2 dispatchThreadId = ThreadGroupTilingX(
        (g_InternalResolution + 7) / 8,
        uint2(8, 8),
        8,
        groupThreadId,
        groupId);

    float3 colorComposite = g_ColorBeforeTransparency_SRV[dispatchThreadId];
    float3 diffuseAlbedoComposite = g_DiffuseAlbedo[dispatchThreadId];
    float3 specularAlbedoComposite = g_SpecularAlbedo[dispatchThreadId];

    uint layerNum = min(GBUFFER_LAYER_NUM, g_LayerNum_SRV[dispatchThreadId]);
    if (layerNum > 1)
    {
        --layerNum;
        Layer layers[GBUFFER_LAYER_NUM - 1];

        for (uint i = 0; i < layerNum; i++)
        {
            GBufferData gBufferData = LoadGBufferData(uint3(dispatchThreadId, i + 1));

            ShadingParams shadingParams = (ShadingParams) 0;
            shadingParams.EyePosition = g_EyePosition.xyz;
            shadingParams.EyeDirection = g_EyePosition.xyz - gBufferData.Position;

            layers[i] = (Layer) 0;
            layers[i].Distance = length(shadingParams.EyeDirection);
            shadingParams.EyeDirection /= layers[i].Distance;

            layers[i].Additive = (gBufferData.Flags & GBUFFER_FLAG_IS_ADDITIVE) != 0;

            if (!(gBufferData.Flags & (GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_SHADOW)))
                shadingParams.Shadow = g_Shadow_SRV[uint3(dispatchThreadId, i + 1)];
            else
                shadingParams.Shadow = 1.0;

            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            {
                shadingParams.GlobalIllumination = g_GlobalIllumination_SRV[uint3(dispatchThreadId, i + 1)];
                layers[i].DiffuseAlbedo = ComputeGI(gBufferData, 1.0);
            }

            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
            {
                shadingParams.Reflection = g_Reflection_SRV[uint3(dispatchThreadId, i + 1)];
                layers[i].SpecularAlbedo = ComputeReflection(gBufferData, 1.0);
            }

            float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;

            if (gBufferData.Flags & (GBUFFER_FLAG_REFRACTION_MUL | GBUFFER_FLAG_REFRACTION_ADD | GBUFFER_FLAG_REFRACTION_OPACITY))
            {
                float2 texCoord = (dispatchThreadId + 0.5) / g_InternalResolution;
                texCoord = texCoord * 2.0 - 1.0;
                texCoord += gBufferData.Normal.xz * 0.05;
                texCoord = texCoord * 0.5 + 0.5;
                shadingParams.Refraction = g_ColorBeforeTransparency_SRV.SampleLevel(g_SamplerState, texCoord, 0);

                if (ComputeDepth(gBufferData.Position, g_MtxView, g_MtxProjection) >= g_Depth_SRV[texCoord * g_InternalResolution])
                    shadingParams.Refraction = colorComposite;

                float3 viewPositionToCompare = mul(
                    float4(LoadGBufferData(uint3(dispatchThreadId, 0)).Position, 1.0), g_MtxView).xyz;

                gBufferData.Alpha *= saturate((viewPosition.z - viewPositionToCompare.z) * 0.5 / 8.0);
            }

            if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                layers[i].Color.rgb = ComputeWaterShading(gBufferData, shadingParams);
            else
                layers[i].Color.rgb = ComputeGeometryShading(gBufferData, shadingParams);

            float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

            if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
                layers[i].Color.rgb = layers[i].Color.rgb * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;

            layers[i].Color.a = gBufferData.Alpha;
        }

        for (uint j = 0; j < layerNum - 1; j++)
        {
            for (uint k = 0; k < layerNum - j - 1; k++)
            {
                if (layers[k].Distance < layers[k + 1].Distance)
                {
                    Layer temp = layers[k + 1];
                    layers[k + 1] = layers[k];
                    layers[k] = temp;
                }
            }
        }

        for (uint i = 0; i < layerNum; i++)
        {
            Layer layer = layers[i];

            if (!layer.Additive)
            {
                colorComposite *= 1.0 - layer.Color.a;
                diffuseAlbedoComposite *= 1.0 - layer.Color.a;
                specularAlbedoComposite *= 1.0 - layer.Color.a;
            }

            colorComposite += layer.Color.rgb * layer.Color.a;
            diffuseAlbedoComposite += layer.DiffuseAlbedo * layer.Color.a;
            specularAlbedoComposite += layer.SpecularAlbedo * layer.Color.a;
        }

        g_DiffuseAlbedo[dispatchThreadId] = diffuseAlbedoComposite;
        g_SpecularAlbedo[dispatchThreadId] = specularAlbedoComposite;
    }

    g_Color[dispatchThreadId] = float4(colorComposite, 1.0);
}