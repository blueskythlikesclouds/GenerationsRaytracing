#pragma once

#include "GBufferData.hlsli"

void CreateFallOffVGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb;
    gBufferData.Alpha *= diffuse.a;

    if (material.NormalTexture != 0)
    {
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

        if (material.NormalTexture2 != 0)
        {
            float3 normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex));
            gBufferData.Normal = NormalizeSafe(gBufferData.Normal + normal);
        }
    }

    if (material.GlossTexture != 0)
    {
        float gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).x;
        gBufferData.Specular *= gloss;
        gBufferData.SpecularEnvironment *= gloss;
        gBufferData.SpecularGloss *= gloss;
    }
    else
    {
        gBufferData.Specular = 0.0;
    }
                
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

    float fresnel = 1.0 - saturate(dot(-WorldRayDirection(), vertex.Normal));
    fresnel *= fresnel;
    gBufferData.Emission = fresnel * vertex.Color.rgb;
}
