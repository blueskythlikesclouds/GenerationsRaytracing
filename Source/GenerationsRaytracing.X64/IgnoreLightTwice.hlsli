#pragma once

#include "GBufferData.hlsli"

void CreateIgnoreLightTwiceGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags =
        GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb * 2.0;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    gBufferData.Emission = gBufferData.Diffuse;
    gBufferData.Diffuse = 0.0;
}
