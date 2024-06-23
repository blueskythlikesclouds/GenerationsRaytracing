#pragma once

#include "GBufferData.hlsli"

void CreateLuminescenceGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
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
    
    gBufferData.SpecularFresnel = 0.4;
    
    gBufferData.Emission = material.DisplacementTexture != 0 ?
        SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb : material.Emissive.rgb;
    
    gBufferData.Emission *= material.Ambient.rgb;
}
