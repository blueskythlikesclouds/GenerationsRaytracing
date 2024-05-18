#pragma once

#include "GBufferData.hlsli"

void CreateCloudGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_HAS_LAMBERT_ADJUSTMENT;

    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    gBufferData.SpecularFresnel = ComputeFresnel(gBufferData.Normal) * 0.7 + 0.3;

    gBufferData.Falloff = ComputeFalloff(gBufferData.Normal, material.SonicSkinFalloffParam.xyz) * vertex.Color.rgb;
    gBufferData.Falloff *= SampleMaterialTexture2D(material.DisplacementTexture, vertex).rgb;

    float3 viewNormal = mul(float4(vertex.Normal, 0.0), g_MtxView).xyz;
    float4 reflection = SampleMaterialTexture2D(material.ReflectionTexture, viewNormal.xy * float2(0.5, -0.5) + 0.5, 0);
    gBufferData.Alpha *= reflection.a * vertex.Color.a;
}
