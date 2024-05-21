#pragma once

#include "GBufferData.hlsli"

void CreateDistortionGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_REFRACTION_ADD;

    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;

    float4 specular = SampleMaterialTexture2D(material.SpecularTexture, vertex);
    gBufferData.SpecularTint *= specular.rgb * vertex.Color.rgb;
    gBufferData.SpecularEnvironment *= specular.a;

    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));

    if (material.NormalTexture2 != 0)
        gBufferData.Normal = NormalizeSafe(gBufferData.Normal + DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex)));

    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.6 + 0.4;

    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    gBufferData.RefractionOffset = viewNormal.xy * 0.05;
}
