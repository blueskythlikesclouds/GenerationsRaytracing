#pragma once

#include "GBufferData.hlsli"

void CreateCommonGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    if (material.OpacityTexture != 0)
        gBufferData.Alpha *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
                
    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
        gBufferData.SpecularEnvironment *= specular.a;
    }

    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }
    else if (material.SpecularTexture == 0)
    {
        gBufferData.Specular = 0.0;
    }
        
    gBufferData.SpecularFresnel = 0.4;

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
}
