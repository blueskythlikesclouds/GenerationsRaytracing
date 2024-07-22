#pragma once

#include "ColorSpace.hlsli"
#include "GBufferData.hlsli"

void CreateBlend2GBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_PBR;
    
    float blend = material.OpacityTexture != 0 ? 
        SampleMaterialTexture2D(material.OpacityTexture, vertex).x : vertex.Color.w;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    float4 diffuse2 = SampleMaterialTexture2D(material.DiffuseTexture2, vertex);
    
    gBufferData.Diffuse = lerp(SrgbToLinear(diffuse.rgb), SrgbToLinear(diffuse2.rgb), blend) * vertex.Color.rgb;
    gBufferData.Alpha = lerp(diffuse.a, diffuse2.a, blend);
    
    if (material.OpacityTexture != 0)
        gBufferData.Alpha *= vertex.Color.a;
    
    float fresnel = 0.0;
    float roughness = 0.0;
    float metalness = 0.0;
    
    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        if (material.SpecularTexture2 != 0)
            specular = lerp(specular, SampleMaterialTexture2D(material.SpecularTexture2, vertex), blend);
        
        fresnel = specular.x / 4.0;
        roughness = max(0.01, 1.0 - specular.y);
        metalness = material.Flags & MATERIAL_FLAG_HAS_METALNESS ? specular.w : specular.x > 0.9;
    }
    else
    {
        fresnel = lerp(material.PBRFactor.x, material.PBRFactor2.x, blend);
        roughness = max(0.01, 1.0 - lerp(material.PBRFactor.y, material.PBRFactor2.y, blend));
    }
    
    gBufferData.Specular = lerp(fresnel, gBufferData.Diffuse, metalness);
    gBufferData.SpecularGloss = roughness + 1.0;
    gBufferData.Diffuse *= 1.0 - metalness;
    
    if (material.NormalTexture != 0)
    {
        float4 normalMap = SampleMaterialTexture2D(material.NormalTexture, vertex);
        if (material.NormalTexture2 != 0)
            normalMap = lerp(normalMap, SampleMaterialTexture2D(material.NormalTexture2, vertex), blend);
        
        gBufferData.Normal = DecodeNormalMap(vertex, normalMap);
    }
}
