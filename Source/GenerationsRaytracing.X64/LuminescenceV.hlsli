#pragma once

#include "GBufferData.hlsli"

void CreateLuminescenceVGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    if (material.DiffuseTexture2 != 0)
        diffuse = lerp(diffuse, SampleMaterialTexture2D(material.DiffuseTexture2, vertex), vertex.Color.w);
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    
    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        if (material.GlossTexture2 != 0)
            gloss = lerp(gloss, SampleMaterialTexture2D(material.GlossTexture2, vertex).x, vertex.Color.w);
    
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }
    else
    {
        gBufferData.Specular = 0.0;
    }
    
    gBufferData.SpecularFresnel = 0.4;
    
    gBufferData.Emission = vertex.Color.rgb * material.Ambient.rgb;
    
    if (material.DisplacementTexture != 0)
        gBufferData.Emission *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;
}
