#pragma once

#include "GeometryDesc.hlsli"
#include "MaterialFlags.h"

struct Material
{
    uint Flags;
    float4 TexCoordOffsets[2];
    uint DiffuseTexture;
    uint DiffuseTexture2;
    uint SpecularTexture;
    uint SpecularTexture2;
    uint GlossTexture;
    uint GlossTexture2;
    uint NormalTexture;
    uint NormalTexture2;
    uint ReflectionTexture;
    uint OpacityTexture;
    uint DisplacementTexture;
    uint DisplacementTexture2;
    uint DisplacementTexture3;
    uint LevelTexture;

    float4 Diffuse;
    float3 Ambient;
    float3 Specular;
    float3 Emissive;
    float2 GlossLevel;
    float Opacity;
    float LuminanceRange;
    float2 FresnelParam;
    float3 SonicEyeHighLightPosition;
    float3 SonicEyeHighLightColor;
    float3 SonicSkinFalloffParam;
    float4 ChrEmissionParam;
    float4 EmissionParam;
    float4 OffsetParam;
    float4 WaterParam;
    float4 FurParam;
    float4 FurParam2;
    float4 ChrEyeFHL1;
    float4 ChrEyeFHL2;
    float4 ChrEyeFHL3;
    float4 DistortionParam;
    float3 IrisColor;
    float4 PupilParam;
    float3 HighLightColor;
    float ChaosWaveParamEx;
    float CloakParam;
    float GlassRefractionParam;
    float2 HeightParam;
};

float4 SampleMaterialTexture2D(uint materialTexture, float2 texCoord, uint level)
{
    uint textureId = materialTexture & 0xFFFFF;
    uint samplerId = (materialTexture >> 20) & 0x3FF;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    return texture.SampleLevel(samplerState, texCoord, level);
}

float4 SampleMaterialTexture2D(uint materialTexture, Vertex vertex, float2 offset = 0.0, float2 scale = 1.0)
{
    uint textureId = materialTexture & 0xFFFFF;
    uint samplerId = (materialTexture >> 20) & 0x3FF;
    uint texCoordIndex = vertex.Flags & VERTEX_FLAG_MULTI_UV ? materialTexture >> 30 : 0;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    if (vertex.Flags & VERTEX_FLAG_MIPMAP)
    {
        return texture.SampleGrad(samplerState, 
            (vertex.TexCoords[texCoordIndex] + offset) * scale,
            vertex.TexCoordsDdx[texCoordIndex] * scale, 
            vertex.TexCoordsDdy[texCoordIndex] * scale);
    }
    else
    {
        return texture.SampleLevel(samplerState, (vertex.TexCoords[texCoordIndex] + offset) * scale, vertex.Flags & VERTEX_FLAG_MIPMAP_LOD ? 2 : 0);
    }
}