#pragma once

#include "GBufferData.hlsli"

void CreateTimeEaterGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    float blend = SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
    float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex);
    float4 normal2 = SampleMaterialTexture2D(material.NormalTexture2, vertex);
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    gBufferData.SpecularTint *= specular.rgb;
    gBufferData.SpecularEnvironment *= specular.a;
    gBufferData.Normal = NormalizeSafe(DecodeNormalMap(vertex, normal) + DecodeNormalMap(vertex, normal2));
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;
    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam);
    
    // TODO: Refraction
}
