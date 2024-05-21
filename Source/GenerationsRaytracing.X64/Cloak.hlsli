#pragma once

#include "GBufferData.hlsli"

void CreateCloakGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = 
        GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION | GBUFFER_FLAG_REFRACTION_MUL;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse = (diffuse.rgb - 1.0) * material.Ambient.x + 1.0;
    gBufferData.Alpha = diffuse.a;
    
    float offset = SampleMaterialTexture2D(material.DisplacementTexture, 
        saturate(dot(vertex.Normal, -WorldRayDirection())) + material.TexCoordOffsets[0].xy, 0).x;
    
    float3 viewNormal = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
    gBufferData.RefractionOffset = viewNormal.xy * offset * material.Ambient.x * material.CloakParam;
}
