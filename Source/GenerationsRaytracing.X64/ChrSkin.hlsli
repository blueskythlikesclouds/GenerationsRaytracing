#pragma once

#include "GBufferData.hlsli"

void CreateChrSkinGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
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
    
    gBufferData.SpecularFresnel = 0.3;

    if (material.NormalTexture != 0)
    {
        float3 normalMap = DecodeNormalMap(SampleMaterialTexture2D(material.NormalTexture, vertex));
        if (material.NormalTexture2 != 0)
            normalMap = BlendNormalMap(normalMap, DecodeNormalMap(SampleMaterialTexture2D(material.NormalTexture2, vertex)));
        
        gBufferData.Normal = DecodeNormalMap(vertex, normalMap);
    }

    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
    if (material.DisplacementTexture != 0)
        gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;
}
