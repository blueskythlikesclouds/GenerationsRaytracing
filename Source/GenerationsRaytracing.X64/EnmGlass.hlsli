#pragma once

#include "GBufferData.hlsli"

void CreateEnmGlassGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

    float4 diffuse = 0.0;

    if (material.DiffuseTexture != 0)
    {
        diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
        gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    }

    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
        gBufferData.SpecularEnvironment *= specular.a;
    }

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);
    gBufferData.Alpha *= saturate(diffuse.a + reflection.a) * vertex.Color.a;

    gBufferData.Emission = material.ChrEmissionParam.rgb;

    if (material.DisplacementTexture != 0)
        gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

    gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;
}
