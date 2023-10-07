#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

struct Material
{
    uint ShaderType;
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

    float4 Diffuse;
    float4 Ambient;
    float4 Specular;
    float4 Emissive;
    float4 PowerGlossLevel;
    float4 OpacityReflectionRefractionSpectype;
    //float4 LuminanceRange;
    float4 FresnelParam;
    float4 SonicEyeHighLightPosition;
    float4 SonicEyeHighLightColor;
    float4 SonicSkinFalloffParam;
    float4 ChrEmissionParam;
    //float4 CloakParam;
    //float4 DistortionParam;
    //float4 GlassRefractionParam;
    //float4 IceParam;
    float4 EmissionParam;
    //float4 OffsetParam;
    //float4 HeightParam;
};

float4 SampleMaterialTexture2D(uint materialTexture, float2 texCoords[4])
{
    uint textureId = materialTexture & 0xFFFFF;
    uint samplerId = (materialTexture >> 20) & 0x3FF;
    uint texCoordIndex = materialTexture >> 30;

    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(textureId)];
    SamplerState samplerState = SamplerDescriptorHeap[NonUniformResourceIndex(samplerId)];

    return texture.SampleLevel(samplerState, texCoords[texCoordIndex], 0);
}

#endif