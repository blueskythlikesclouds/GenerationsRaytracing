#pragma once

#include "GBufferData.hlsli"

void CreateIgnoreLightGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags =
        GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    
    if (material.OpacityTexture != 0)
        gBufferData.Alpha *= SampleMaterialTexture2D(material.OpacityTexture, vertex).x;
    
    if (material.DisplacementTexture != 0)
    {
        gBufferData.Emission = SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;
        gBufferData.Emission += material.EmissionParam.rgb;
        gBufferData.Emission *= material.Ambient.rgb * material.EmissionParam.w;
    }
    gBufferData.Emission += gBufferData.Diffuse;
    gBufferData.Diffuse = 0.0;
}
