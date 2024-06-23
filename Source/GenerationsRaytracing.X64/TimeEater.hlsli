#pragma once

#include "GBufferData.hlsli"

void CreateTimeEaterGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_MUL_BY_REFRACTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex);
    float4 normal2 = SampleMaterialTexture2D(material.NormalTexture2, vertex);
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    gBufferData.SpecularTint *= specular.rgb;
    gBufferData.SpecularEnvironment *= specular.a;
    gBufferData.SpecularFresnel = 0.3;
    gBufferData.Normal = NormalizeSafe(DecodeNormalMap(vertex, normal) + DecodeNormalMap(vertex, normal2));
    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam);

    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    gBufferData.Refraction = SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
    gBufferData.RefractionOffset = viewNormal.xy * material.ChaosWaveParamEx;
}
