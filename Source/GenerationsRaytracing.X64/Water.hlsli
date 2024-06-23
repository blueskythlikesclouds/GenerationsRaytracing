#pragma once

#include "GBufferData.hlsli"

void CreateWaterGBufferData(Vertex vertex, Material material, inout GBufferData gBufferData)
{
    gBufferData.Flags = GBUFFER_FLAG_IGNORE_LOCAL_LIGHT |
        GBUFFER_FLAG_IS_MIRROR_REFLECTION | GBUFFER_FLAG_IS_WATER;

    float2 offset = material.WaterParam.xy * g_TimeParam.y * 0.08;
    float4 decal = SampleMaterialTexture2D(material.DiffuseTexture, vertex, float2(0.0, offset.x));
    
    gBufferData.Diffuse = decal.rgb * vertex.Color.rgb;
    gBufferData.Alpha = decal.a * vertex.Color.a;

    float3 normal1 = DecodeNormalMap(SampleMaterialTexture2D(material.NormalTexture, vertex, float2(0.0, offset.x)));
    float3 normal2 = DecodeNormalMap(SampleMaterialTexture2D(material.NormalTexture2, vertex, float2(0.0, offset.y)));
    float3 normal = NormalizeSafe((normal1 + normal2) * 0.5);
    
    gBufferData.Normal = NormalizeSafe(DecodeNormalMap(vertex, normal));
    
    gBufferData.RefractionOffset = normal.xy;
    
#if SHADER_TYPE == SHADER_TYPE_WATER_ADD 
    gBufferData.Flags |= GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_REFRACTION_ADD;

    if (material.Flags & MATERIAL_FLAG_SOFT_EDGE)
    {
        float3 viewPosition = mul(float4(vertex.Position, 0.0), g_MtxView).xyz;
        gBufferData.Alpha *= smoothstep(0.0, 1.0, saturate(viewPosition.z - LinearizeDepth(g_Depth[DispatchRaysIndex().xy], g_MtxInvProjection)));
    }

    gBufferData.Refraction = 1.0;
    gBufferData.RefractionOffset *= 0.05;
    
#elif SHADER_TYPE == SHADER_TYPE_WATER_MUL 
    gBufferData.Flags |= GBUFFER_FLAG_IGNORE_DIFFUSE_LIGHT | GBUFFER_FLAG_REFRACTION_MUL;
    
    if (material.Flags & MATERIAL_FLAG_SOFT_EDGE)
    {
        float3 viewPosition = mul(float4(vertex.Position, 0.0), g_MtxView).xyz;
        gBufferData.Alpha *= smoothstep(0.0, 1.0, saturate(viewPosition.z - LinearizeDepth(g_Depth[DispatchRaysIndex().xy], g_MtxInvProjection)));
    }
    
    gBufferData.RefractionOffset *= 0.05;

#elif SHADER_TYPE == SHADER_TYPE_WATER_OPACITY
    gBufferData.Flags |= GBUFFER_FLAG_REFRACTION_OPACITY;
    gBufferData.Refraction = gBufferData.Alpha;
    
    if (material.Flags & MATERIAL_FLAG_SOFT_EDGE)
    {
        float3 viewPosition = mul(float4(vertex.Position, 0.0), g_MtxView).xyz;
        gBufferData.Alpha = smoothstep(0.0, 1.0, saturate((viewPosition.z - LinearizeDepth(g_Depth[DispatchRaysIndex().xy], g_MtxInvProjection)) / material.WaterParam.w));
    }
    
    gBufferData.RefractionOffset *= 0.05 + material.WaterParam.z;   
    
#endif
}
