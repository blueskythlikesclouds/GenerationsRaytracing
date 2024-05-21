#pragma once

#include "GBufferData.hlsli"

void CreateDistortionOverlayGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    gBufferData.Flags =
        GBUFFER_FLAG_IGNORE_GLOBAL_LIGHT | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION | GBUFFER_FLAG_IGNORE_REFLECTION | GBUFFER_FLAG_REFRACTION_OVERLAY;
    
    float4 diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex);
    gBufferData.Diffuse = diffuse.rgb;
    gBufferData.Alpha *= diffuse.a * vertex.Color.a;
    gBufferData.Specular = vertex.Color.rgb;
    gBufferData.Normal = DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture, vertex));
    
    if (material.NormalTexture2 != 0)
        gBufferData.Normal = NormalizeSafe(gBufferData.Normal + DecodeNormalMap(vertex, SampleMaterialTexture2D(material.NormalTexture2, vertex)));
    
    float3 viewPosition = mul(float4(vertex.Position, 0.0), g_MtxView).xyz;
    viewPosition.z -= material.DistortionParam.x;
    
    float depth = ComputeDepth(viewPosition, g_MtxProjection);
    gBufferData.Refraction = max(0.0, g_Depth[DispatchRaysIndex().xy] - depth) * (material.DistortionParam.y / (1.0 - depth));
    
    float3 viewNormal = mul(float4(gBufferData.Normal, 0.0), g_MtxView).xyz;
    gBufferData.RefractionOffset = viewNormal.xy * material.DistortionParam.w;
    gBufferData.RefractionOverlay = material.DistortionParam.z;
}
