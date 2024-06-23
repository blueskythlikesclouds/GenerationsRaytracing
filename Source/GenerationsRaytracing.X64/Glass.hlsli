#pragma once

#include "GBufferData.hlsli"

void CreateGlassGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IS_MIRROR_REFLECTION;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    
    gBufferData.SpecularTint *= material.FresnelParam.y;
    gBufferData.SpecularFresnel = material.FresnelParam.x;
    
    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        gBufferData.SpecularTint *= specular.rgb;
        gBufferData.SpecularEnvironment *= specular.a;
    }
    
    if (material.GlossTexture != 0)
    {
        float4 gloss = SampleMaterialTexture2D(material.GlossTexture, vertex);
        if (material.SpecularTexture != 0)
        {
            gBufferData.Specular *= gloss.x;
            gBufferData.SpecularEnvironment *= gloss.x;
            gBufferData.SpecularLevel *= gloss.x;
        }
        else
        {
            gBufferData.SpecularTint *= gloss.rgb;
            gBufferData.SpecularEnvironment *= gloss.w;
        }
    }
    else if (material.SpecularTexture == 0)
    {
        gBufferData.Specular = 0.0;
    }
    
    float3 specularFresnel = ComputeFresnel(gBufferData.SpecularFresnel, dot(gBufferData.Normal, -WorldRayDirection()));
    
    float3 visibilityFactor = gBufferData.SpecularTint * gBufferData.SpecularEnvironment * specularFresnel * 0.5;
    gBufferData.Alpha = sqrt(max(gBufferData.Alpha * gBufferData.Alpha, dot(visibilityFactor, visibilityFactor)));
}
