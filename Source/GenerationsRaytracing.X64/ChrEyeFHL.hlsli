#pragma once

#include "GBufferData.hlsli"

#if SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL_PROCEDURAL
#include "ProceduralEye.hlsli"
#endif

void CreateChrEyeFHLGBufferData(Vertex vertex, Material material, InstanceDesc instanceDesc, inout GBufferData gBufferData)
{
    float3 direction = mul(instanceDesc.HeadTransform, float4(0.0, 0.0, 1.0, 0.0));
    direction = NormalizeSafe(mul(float4(direction, 0.0), g_MtxView).xyz);
    float2 offset = material.TexCoordOffsets[0].xy * 2.0 + direction.xy * float2(-1.0, 1.0);
    
    float2 pupilOffset = -material.ChrEyeFHL1.zw * offset;
    float2 highLightOffset = material.ChrEyeFHL3.xy + material.ChrEyeFHL3.zw * offset;
    float2 catchLightOffset = material.ChrEyeFHL1.xy - offset * float2(
        offset.x < 0 ? material.ChrEyeFHL2.x : material.ChrEyeFHL2.y,
        offset.y < 0 ? material.ChrEyeFHL2.z : material.ChrEyeFHL2.w);
    
    float3 diffuse;
    float pupil;
    float3 highLight;
    float catchLight;
    float4 mask;
    
#if SHADER_TYPE == SHADER_TYPE_CHR_EYE_FHL_PROCEDURAL
    GenerateProceduralEye(
        vertex.TexCoords[0],
        material.IrisColor,
        vertex.TexCoords[0] + pupilOffset,
        material.PupilParam.xy,
        material.PupilParam.z,
        material.PupilParam.w,
        vertex.TexCoords[0] + highLightOffset,
        material.HighLightColor,
        vertex.TexCoords[0] + catchLightOffset,
        diffuse,
        pupil,
        highLight,
        catchLight,
        mask);
#else
    diffuse = SampleMaterialTexture2D(material.DiffuseTexture, vertex).rgb;
    pupil = SampleMaterialTexture2D(material.DiffuseTexture, vertex, pupilOffset).w;
    highLight = SampleMaterialTexture2D(material.LevelTexture, vertex, highLightOffset).rgb;
    catchLight = SampleMaterialTexture2D(material.LevelTexture, vertex, catchLightOffset).w;
    mask = SampleMaterialTexture2D(material.DisplacementTexture, vertex);
#endif
    
    gBufferData.Diffuse *= diffuse * pupil * (1.0 - catchLight);
    gBufferData.Specular *= pupil * mask.b * vertex.Color.w * (1.0 - catchLight);
    gBufferData.SpecularFresnel = ComputeFresnel(vertex.Normal) * 0.7 + 0.3;
    gBufferData.Emission = (highLight * pupil * mask.w * (1.0 - catchLight) + catchLight) / GetExposure();
}
