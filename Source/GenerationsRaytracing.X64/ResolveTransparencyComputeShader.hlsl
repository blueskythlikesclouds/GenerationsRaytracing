#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"
#include "ThreadGroupTilingX.hlsl"

struct Layer
{
    float4 Color;
    float3 DiffuseAlbedo;
    float3 SpecularAlbedo;
    float4 NormalsRoughness;
    float SpecularHitDistance;
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

    float4 colorComposite = g_ColorBeforeTransparency_SRV[dispatchThreadId];
    float3 diffuseAlbedoComposite = g_DiffuseAlbedo[dispatchThreadId];
    float3 specularAlbedoComposite = g_SpecularAlbedo[dispatchThreadId];
    float4 normalsRoughnessComposite = g_NormalsRoughness[dispatchThreadId];
    float specularHitDistanceComposite = g_SpecularHitDistance[dispatchThreadId];

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
                shadingParams.GlobalIllumination = g_GlobalIllumination_SRV[uint3(dispatchThreadId, i + 1)].rgb;
                layers[i].DiffuseAlbedo = ComputeGI(gBufferData, 1.0);
            }

            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
            {
                float4 reflectionAndHitDistance = g_Reflection_SRV[uint3(dispatchThreadId, i + 1)];
                shadingParams.Reflection = reflectionAndHitDistance.rgb;
                layers[i].SpecularAlbedo = ComputeReflection(gBufferData, 1.0);
                layers[i].SpecularHitDistance = reflectionAndHitDistance.w;
            }
            
            layers[i].NormalsRoughness = float4(gBufferData.Normal, 0.0);
            if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_GLASS_REFLECTION)))
                layers[i].NormalsRoughness.w = ConvertSpecularGlossToRoughness(gBufferData.SpecularGloss);

            float3 viewPosition = mul(float4(gBufferData.Position, 1.0), g_MtxView).xyz;

            if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_ALL)
            {
                float2 texCoord = (dispatchThreadId + 0.5) / g_InternalResolution;
                texCoord = texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0);
                texCoord += gBufferData.RefractionOffset;
                texCoord = texCoord * float2(0.5, -0.5) + 0.5;

                if (ComputeDepth(gBufferData.Position, g_MtxView, g_MtxProjection) < g_Depth_SRV[texCoord * g_InternalResolution])
                    shadingParams.Refraction = g_ColorBeforeTransparency_SRV.SampleLevel(g_SamplerState, texCoord, 0).rgb;
                else 
                    shadingParams.Refraction = colorComposite.rgb;
            }

            if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                layers[i].Color.rgb = ComputeWaterShading(gBufferData, shadingParams);
            else
                layers[i].Color.rgb = ComputeGeometryShading(gBufferData, shadingParams);

            float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

            if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
            {
                layers[i].Color.rgb = layers[i].Color.rgb * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
                layers[i].DiffuseAlbedo = layers[i].DiffuseAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
                layers[i].SpecularAlbedo = layers[i].SpecularAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
            }

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
                normalsRoughnessComposite *= 1.0 - layer.Color.a;
                specularHitDistanceComposite *= 1.0 - layer.Color.a;
            }

            colorComposite += layer.Color * layer.Color.a;
            diffuseAlbedoComposite += layer.DiffuseAlbedo * layer.Color.a;
            specularAlbedoComposite += layer.SpecularAlbedo * layer.Color.a;
            
            if (!layer.Additive)
            {
                normalsRoughnessComposite += layer.NormalsRoughness * layer.Color.a;
                specularHitDistanceComposite += layer.SpecularHitDistance * layer.Color.a;
            }
        }
        
        normalsRoughnessComposite.rgb = NormalizeSafe(normalsRoughnessComposite.rgb);

        g_DiffuseAlbedo[dispatchThreadId] = diffuseAlbedoComposite;
        g_SpecularAlbedo[dispatchThreadId] = specularAlbedoComposite;
        g_NormalsRoughness[dispatchThreadId] = normalsRoughnessComposite;
        g_SpecularHitDistance[dispatchThreadId] = specularHitDistanceComposite;
    }

    g_Color[dispatchThreadId] = colorComposite;
}