#ifndef MATERIAL_DATA_HLSLI_INCLUDED
#define MATERIAL_DATA_HLSLI_INCLUDED

#include "GeometryDesc.hlsli"

struct Material
{
    uint ShaderType : 16;
    uint Flags : 16;
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

    float4 TexCoordOffsets[2];
    float4 Diffuse;
    float3 Ambient;
    float3 Specular;
    float3 Emissive;
    float3 PowerGlossLevel;
    float OpacityReflectionRefractionSpectype;
    float LuminanceRange;
    float2 FresnelParam;
    float3 SonicEyeHighLightPosition;
    float3 SonicEyeHighLightColor;
    float3 SonicSkinFalloffParam;
    float4 ChrEmissionParam;
    float3 TransColorMask;
    float4 EmissionParam;
    float2 OffsetParam;
    float2 WaterParam;
};

float4 SampleMaterialTexture2D(uint materialTexture, float2 texCoord)
{
    uint textureId = materialTexture & 0xFFFFF;
    uint samplerId = (materialTexture >> 20) & 0x3FF;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    return texture.SampleLevel(samplerState, texCoord, 0);
}

float4 SampleMaterialTexture2D(uint materialTexture, Vertex vertex)
{
    uint textureId = materialTexture & 0xFFFFF;
    uint samplerId = (materialTexture >> 20) & 0x3FF;
    uint texCoordIndex = vertex.Flags & VERTEX_FLAG_MULTI_UV ? materialTexture >> 30 : 0;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    if (vertex.Flags & VERTEX_FLAG_MIPMAP)
    {
        return texture.SampleGrad(samplerState, 
            vertex.TexCoords[texCoordIndex], 
            vertex.TexCoordsDdx[texCoordIndex], 
            vertex.TexCoordsDdy[texCoordIndex]);
    }
    else
    {
        return texture.SampleLevel(samplerState, vertex.TexCoords[texCoordIndex], 2);
    }
}

#endif