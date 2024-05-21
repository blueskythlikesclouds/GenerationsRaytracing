#pragma once

#include "GBufferData.hlsli"

void CreateFallOffGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }
    else
    {
        gBufferData.Specular = 0.0;
    }
                
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

    float3 viewNormal = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
    gBufferData.Emission = SampleMaterialTexture2D(material.DisplacementTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0).rgb;
}
