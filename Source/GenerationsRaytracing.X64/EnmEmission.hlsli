#pragma once

#include "GBufferData.hlsli"

void CreateEnmEmissionGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    if (material.NormalTexture != 0)
        gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    if (material.SpecularTexture != 0)
    {
        float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
        gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
        gBufferData.SpecularEnvironment *= specular.a;
    }
    else
    {
        gBufferData.Specular = 0.0;
    }
            
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

    gBufferData.Emission = material.ChrEmissionParam.rgb;

    if (material.DisplacementTexture != 0)
        gBufferData.Emission += SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

    gBufferData.Emission *= material.Ambient.rgb * material.ChrEmissionParam.w * vertex.Color.rgb;
}
