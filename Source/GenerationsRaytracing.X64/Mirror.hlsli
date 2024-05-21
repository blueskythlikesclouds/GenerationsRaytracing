#pragma once

#include "GBufferData.hlsli"

void CreateMirrorGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    gBufferData.Specular = 0.0;
    gBufferData.SpecularFresnel = ComputeFresnel(vertex.Normal, material.FresnelParam.xy);
}
