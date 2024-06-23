#pragma once

#include "GBufferData.hlsli"

void CreateIndirectGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 offset = SampleMaterialTexture2D(material.DisplacementTexture, vertex);
    offset.xy = (offset.wx * 2.0 - 1.0) * material.OffsetParam.xy;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset.xy);
    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex, offset.xy).x;
    
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    gBufferData.Specular *= gloss;
    gBufferData.SpecularEnvironment *= gloss;
    gBufferData.SpecularGloss *= gloss;
    gBufferData.SpecularFresnel = 0.4;
    
    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex, offset.xy));
}
