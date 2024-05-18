#pragma once

#include "GBufferData.hlsli"

void CreateFadeOutNormalGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;

    float3 normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    gBufferData.Normal = NormalizeSafe(lerp(normal, gBufferData.Normal, vertex.Color.x));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
}
