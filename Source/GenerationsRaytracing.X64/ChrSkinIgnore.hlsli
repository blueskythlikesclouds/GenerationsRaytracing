#pragma once

#include "GBufferData.hlsli"

void CreateChrSkinIgnoreGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT;

    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
    gBufferData.SpecularEnvironment *= specular.a;
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;

    gBufferData.Emission = material.ChrEmissionParam.rgb;

    if (material.DisplacementTexture != 0)
        gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

    gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;

    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);

    gBufferData.Diffuse *= reflection.rgb;
    gBufferData.Emission += gBufferData.Diffuse;
}
