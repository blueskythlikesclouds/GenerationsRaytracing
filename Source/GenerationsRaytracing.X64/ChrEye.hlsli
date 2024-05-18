#pragma once

#include "GBufferData.hlsli"

void CreateChrEyeGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    float2 gloss = SampleMaterialTexture2D(material.GlossTexture, vertex).xy;
    float4 normalMap = SampleMaterialTexture2D(material.NormalTexture, vertex);
    
    float3 highlightPosition = mul(instanceDesc.HeadTransform, float4(material.SonicEyeHighLightPosition, 1.0));
    float3 highlightNormal = DecodeNormalMap(vertex, float4(normalMap.xy, 0.0, 1.0));
    float3 lightDirection = NormalizeSafe(vertex.Position - highlightPosition);
    float3 halfwayDirection = NormalizeSafe(-WorldRayDirection() + lightDirection);
    
    float highlightSpecular = saturate(dot(highlightNormal, halfwayDirection));
    highlightSpecular = pow(highlightSpecular, max(1.0, min(1024.0, gBufferData.SpecularGloss * gloss.x)));
    highlightSpecular *= gBufferData.SpecularLevel * gloss.x;
    highlightSpecular *= ComputeFresnel(vertex.Normal) * 0.7 + 0.3;
    
    gBufferData.Diffuse *= diffuse.rgb * vertex.Color.rgb;
    gBufferData.Alpha *= diffuse.a;
    
    gBufferData.Specular *= gloss.y * vertex.Color.a;
    gBufferData.SpecularEnvironment *= gloss.y * vertex.Color.a;
    gBufferData.SpecularGloss *= gloss.y;
    gBufferData.SpecularFresnel = ComputeFresnel(DecodeNormalMap(vertex, float4(normalMap.zw, 0.0, 1.0))) * 0.7 + 0.3;
    gBufferData.Emission = highlightSpecular * material.SonicEyeHighLightColor.rgb;
}
