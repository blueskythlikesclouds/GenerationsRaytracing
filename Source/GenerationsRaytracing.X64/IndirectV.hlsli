#pragma once

#include "GBufferData.hlsli"

void CreateIndirectVGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 offset = SampleMaterialTexture2D(material.DisplacementTexture, vertex);
    
    offset.xy = (offset.wx * 2.0 - 1.0) * material.OffsetParam.xy * vertex.Color.w;
    offset.xy *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset.xy);
    float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex, offset.xy).x;
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    gBufferData.Specular *= gloss;
    gBufferData.SpecularEnvironment *= gloss;
    gBufferData.SpecularGloss *= gloss;
    
    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex, offset.xy));
    
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;
    
    gBufferData.Emission = vertex.Color.rgb;
}
