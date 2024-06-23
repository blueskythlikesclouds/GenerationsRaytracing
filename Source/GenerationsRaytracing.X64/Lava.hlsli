#pragma once

#include "GBufferData.hlsli"

void CreateLavaGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IGNORE_SHADOW | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION;
    
    float2 offset = SampleMaterialTexture2D(material.DisplacementTexture3, vertex).wx;
    offset *= material.OffsetParam.xy;
    
    float height = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset).w;
    height = (height * 2.0 - 1.0) * material.HeightParam.y + material.HeightParam.x;
    
    float3 heightVector = NormalizeSafe(float3(
        dot(vertex.Tangent, -WorldRayDirection()),
        dot(vertex.Binormal, -WorldRayDirection()),
        dot(vertex.Normal, -WorldRayDirection())));
    
    offset = heightVector.xy * height;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex, offset);
    float4 normal = SampleMaterialTexture2D(material.NormalTexture, vertex, offset);
    
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.SpecularFresnel = 0.4;
    gBufferData.Normal = DecodeNormalMap(vertex, normal);
    
    offset = SampleMaterialTexture2D(material.DisplacementTexture2, vertex).wx;
    offset *= material.OffsetParam.zw;
    
    float4 emission = SampleMaterialTexture2D(material.DisplacementTexture, vertex, offset);
    gBufferData.Emission = emission.rgb * emission.w + emission.rgb;
    
    float fadeFactor = saturate(diffuse.w * 2.0 - 1.0);
    gBufferData.Diffuse *= fadeFactor;
    gBufferData.Specular *= fadeFactor;
    gBufferData.Emission *= 1.0 - fadeFactor;
}
