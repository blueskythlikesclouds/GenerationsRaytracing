#pragma once

#include "GBufferData.hlsli"

void CreateDimGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;

    gBufferData.Emission = material.Ambient.rgb * vertex.Color.rgb *
        SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0).rgb;
}
