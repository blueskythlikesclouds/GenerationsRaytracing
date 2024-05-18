#pragma once

#include "GBufferData.hlsli"

void CreateIndirectNoLightGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_REFLECTION;
    
    float4 offset = SampleMaterialTexture2D(material.DisplacementTexture, vertex);
    offset.xy = (offset.wx * 2.0 - 1.0) * material.OffsetParam.xy * 2.0;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset.xy);
    float4 emission = SampleMaterialTexture2D(material.DisplacementTexture2, vertex, offset.xy);
    
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    gBufferData.Emission = gBufferData.Diffuse;
    gBufferData.Emission += emission.rgb * vertex.Color.rgb;
}
