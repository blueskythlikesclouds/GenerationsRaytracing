#pragma once

#include "ColorSpace.hlsli"
#include "GBufferData.hlsli"

void CreateCommon2GBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_PBR;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    
    gBufferData.Diffuse = SrgbToLinear(diffuse.rgb) * vertex.Color.rgb;
    gBufferData.Alpha = diffuse.a * vertex.Color.a;
    
    float fresnel = 0.0;
    float roughness = 0.0;
    float metalness = 0.0;
    
    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        
        fresnel = specular.x / 4.0;
        roughness = max(0.01, 1.0 - specular.y);
        metalness = material.Flags & MATERIAL_FLAG_HAS_METALNESS ? specular.w : specular.x > 0.9;
    }
    else
    {
        fresnel = material.PBRFactor.x;
        roughness = max(0.01, 1.0 - material.PBRFactor.y);
    }
    
    gBufferData.Specular = lerp(fresnel, gBufferData.Diffuse, metalness);
    gBufferData.SpecularGloss = roughness + 1.0;
    gBufferData.Diffuse *= 1.0 - metalness;
    
    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    
    if (material.TransparencyTexture != 0)
        gBufferData.Alpha *= SampleMaterialTexture2D(material.TransparencyTexture, vertex).x;
}
