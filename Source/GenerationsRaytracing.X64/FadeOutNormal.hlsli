#pragma once

#include "GBufferData.hlsli"

void CreateFadeOutNormalGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;

    gBufferData.SpecularFresnel = 0.4;
    
    float3 normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    gBufferData.Normal = NormalizeSafe(lerp(normal, gBufferData.Normal, vertex.Color.x));
}
