#pragma once

#include "GBufferData.hlsli"

void CreateChrSkinGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);

    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
    gBufferData.SpecularEnvironment *= specular.a;

    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
    if (material.DisplacementTexture != 0)
        gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;
}
