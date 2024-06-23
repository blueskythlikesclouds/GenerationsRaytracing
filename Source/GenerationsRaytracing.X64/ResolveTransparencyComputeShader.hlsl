#include "GBufferData.h"
#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"
#include "ThreadGroupTilingX.hlsl"

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
    float3 diffuseAlbedoComposite = g_DiffuseAlbedoBeforeTransparency_SRV[dispatchThreadId];
    float3 specularAlbedoComposite = g_SpecularAlbedoBeforeTransparency_SRV[dispatchThreadId];

    uint layerNum = min(GBUFFER_LAYER_NUM, g_LayerNum_SRV[dispatchThreadId]);
    if (layerNum > 1)
    {
        float4 normalsRoughnessComposite = g_NormalsRoughness[dispatchThreadId];
        float specularHitDistanceComposite = g_SpecularHitDistance[dispatchThreadId];

        uint randSeed = InitRandom(dispatchThreadId.xy);
        
        for (uint i = 1; i < layerNum; i++)
        {
            GBufferData gBufferData = LoadGBufferData(uint3(dispatchThreadId, i));

            ShadingParams shadingParams = (ShadingParams) 0;
            shadingParams.EyeDirection = NormalizeSafe(-gBufferData.Position);

            float3 diffuseAlbedo = gBufferData.Emission;
            
            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION))
            {
                shadingParams.GlobalIllumination = g_GlobalIllumination_SRV[uint3(dispatchThreadId, i)].rgb;
                diffuseAlbedo += ComputeGI(gBufferData, 1.0);
            }

            float3 specularAlbedo = 0.0;
            float specularHitDistance = 0.0;
            
            if (!(gBufferData.Flags & GBUFFER_FLAG_IGNORE_REFLECTION))
            {
                float4 reflectionAndHitDistance = g_Reflection_SRV[uint3(dispatchThreadId, i)];
                shadingParams.Reflection = reflectionAndHitDistance.rgb;
                specularAlbedo = ComputeSpecularAlbedo(gBufferData, shadingParams.EyeDirection);
                specularHitDistance = reflectionAndHitDistance.w;
            }
            
            float4 normalsRoughness = float4(gBufferData.Normal, 0.0);
            if (!(gBufferData.Flags & GBUFFER_FLAG_IS_MIRROR_REFLECTION))
                normalsRoughness.w = ConvertSpecularGlossToRoughness(gBufferData.SpecularGloss);

            float3 viewPosition = mul(float4(gBufferData.Position, 0.0), g_MtxView).xyz;

            if (gBufferData.Flags & GBUFFER_FLAG_REFRACTION_ALL)
            {
                float2 texCoord = (dispatchThreadId + 0.5) / g_InternalResolution;
                texCoord = texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0);
                texCoord += gBufferData.RefractionOffset;
                texCoord = texCoord * float2(0.5, -0.5) + 0.5;

                float3 diffuseAlbedoRefraction;
                float3 specularAlbedoRefraction;
                
                if (ComputeDepth(gBufferData.Position, g_MtxView, g_MtxProjection) < g_Depth_SRV[texCoord * g_InternalResolution])
                {
                    shadingParams.Refraction = g_ColorBeforeTransparency_SRV[texCoord * g_InternalResolution].rgb;
                    diffuseAlbedoRefraction = g_DiffuseAlbedoBeforeTransparency_SRV[texCoord * g_InternalResolution].rgb;
                    specularAlbedoRefraction = g_SpecularAlbedoBeforeTransparency_SRV[texCoord * g_InternalResolution].rgb;
                }
                else
                {
                    shadingParams.Refraction = colorComposite.rgb;
                    diffuseAlbedoRefraction = diffuseAlbedoComposite;
                    specularAlbedoRefraction = specularAlbedoComposite;
                }
                
                diffuseAlbedo += ComputeRefraction(gBufferData, diffuseAlbedoRefraction);
                specularAlbedo += ComputeRefraction(gBufferData, specularAlbedoRefraction);
            }
            
            float3 color;

            if (gBufferData.Flags & GBUFFER_FLAG_IS_WATER)
                color = ComputeWaterShading(gBufferData, shadingParams, randSeed);
            else
                color = ComputeGeometryShading(gBufferData, shadingParams, randSeed);

            float2 lightScattering = ComputeLightScattering(gBufferData.Position, viewPosition);

            if (all(and(!isnan(lightScattering), !isinf(lightScattering))))
            {
                color = color * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
                diffuseAlbedo = diffuseAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
                specularAlbedo = specularAlbedo * lightScattering.x + g_LightScatteringColor.rgb * lightScattering.y;
            }

            if (!(gBufferData.Flags & GBUFFER_FLAG_IS_ADDITIVE))
            {
                colorComposite *= 1.0 - gBufferData.Alpha;
                diffuseAlbedoComposite *= 1.0 - gBufferData.Alpha;
                specularAlbedoComposite *= 1.0 - gBufferData.Alpha;
                normalsRoughnessComposite *= 1.0 - gBufferData.Alpha;
                specularHitDistanceComposite *= 1.0 - gBufferData.Alpha;
            }

            colorComposite += float4(color, gBufferData.Alpha) * gBufferData.Alpha;
            diffuseAlbedoComposite += diffuseAlbedo * gBufferData.Alpha;
            specularAlbedoComposite += specularAlbedo * gBufferData.Alpha;
            
            if (!(gBufferData.Flags & GBUFFER_FLAG_IS_ADDITIVE))
            {
                normalsRoughnessComposite += normalsRoughness * gBufferData.Alpha;
                specularHitDistanceComposite += specularHitDistance * gBufferData.Alpha;
            }
        }

        normalsRoughnessComposite.rgb = NormalizeSafe(normalsRoughnessComposite.rgb);

        g_NormalsRoughness[dispatchThreadId] = normalsRoughnessComposite;
        g_SpecularHitDistance[dispatchThreadId] = specularHitDistanceComposite;
    }
    
    if (g_EnableAccumulation)
        colorComposite = lerp(g_Color[dispatchThreadId], colorComposite, 1.0 / float(g_CurrentFrame + 1));

    g_Color[dispatchThreadId] = colorComposite;
    g_DiffuseAlbedo[dispatchThreadId] = diffuseAlbedoComposite;
    g_SpecularAlbedo[dispatchThreadId] = specularAlbedoComposite;
}