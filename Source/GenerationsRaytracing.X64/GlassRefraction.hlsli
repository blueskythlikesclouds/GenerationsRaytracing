#pragma once

#include "GBufferData.hlsli"

void CreateGlassRefractionGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_REFRACTION_OPACITY | GBUFFER_FLAG_MUL_BY_REFRACTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha = material.Opacity;
    
    gBufferData.SpecularTint *= material.FresnelParam.y;
    gBufferData.SpecularEnvironment *= 1.0 - material.Diffuse.w * diffuse.w;
    gBufferData.SpecularFresnel = material.FresnelParam.x;
        
    float r2w = material.GlassRefractionParam * material.GlassRefractionParam;
    float3 r3 = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
    float r4w = 1.0 - r3.z * r3.z;
    r2w = 1.0 - r2w * r4w;
    r4w = r2w >= 0.0 ? 1.0 : 0.0;
    r2w = r2w * r4w;
    r4w = r4w * material.GlassRefractionParam;
    r2w = sqrt(r2w);
    r2w = r2w - r4w * r3.z;
    float2 r5xy = r3.xy * r2w * -0.707106769;
    
    gBufferData.Refraction = material.Diffuse.w * diffuse.w;
    gBufferData.RefractionOffset = r5xy;
}
