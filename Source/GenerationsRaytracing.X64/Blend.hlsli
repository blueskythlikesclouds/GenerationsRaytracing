#pragma once

#include "GBufferData.hlsli"

void CreateBlendGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float blendFactor = material.OpacityTexture != 0 ?
        SampleMaterialTexture2D(material.OpacityTexture, vertex).x : vertex.Color.x;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    if (material.DiffuseTexture2 != 0)
        diffuse = lerp(diffuse, SampleMaterialTexture2D(material.DiffuseTexture2, vertex), blendFactor);
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        if (material.SpecularTexture2 != 0)
            specular = lerp(specular, SampleMaterialTexture2D(material.SpecularTexture2, vertex), blendFactor);
    
        gBufferData.SpecularTint *= specular.rgb;
        gBufferData.SpecularEnvironment *= specular.a;
    }
    
    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        if (material.GlossTexture2 != 0)
            gloss = lerp(gloss, SampleMaterialTexture2D(material.GlossTexture2, vertex).x, blendFactor);
    
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
    {
        float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex);
        if (material.NormalTexture2 != 0)
            normal = lerp(normal, SampleMaterialTexture2D(material.NormalTexture2, vertex), blendFactor);
    
        gBufferData.Normal = DecodeNormalMap(vertex, normal);
    }
}
