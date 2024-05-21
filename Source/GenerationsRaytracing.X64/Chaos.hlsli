#pragma once

#include "GBufferData.hlsli"

void CreateChaosGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_MUL_BY_REFRACTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
                
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    gBufferData.SpecularTint *= specular.rgb;
    gBufferData.SpecularEnvironment *= specular.a;

    gBufferData.Normal = NormalizeSafe(
        DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex)) +
        DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex)));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;
    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz);
    
    if (material.OpacityTexture != 0)
    {
        gBufferData.Alpha *= vertex.Color.a;
        gBufferData.Refraction = SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
    }
    else
    {
        gBufferData.Diffuse *= vertex.Color.rgb;
        gBufferData.SpecularTint *= vertex.Color.rgb;
        gBufferData.Falloff *= vertex.Color.rgb;
        gBufferData.Refraction = vertex.Color.a;
    }
    
    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    gBufferData.RefractionOffset = viewNormal.xy * material.ChaosWaveParamEx;
    
}