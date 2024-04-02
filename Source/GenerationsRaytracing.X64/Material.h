#pragma once

struct Material
{
    uint32_t shaderType : 16;
    uint32_t flags : 16;
    uint32_t diffuseTexture;
    uint32_t diffuseTexture2;
    uint32_t specularTexture;
    uint32_t specularTexture2;
    uint32_t glossTexture;
    uint32_t glossTexture2;
    uint32_t normalTexture;
    uint32_t normalTexture2;
    uint32_t reflectionTexture;
    uint32_t opacityTexture;
    uint32_t displacementTexture;
    uint32_t levelTexture;

    float texCoordOffsets[8];
    float diffuse[4];
    float ambient[3];
    float specular[3];
    float emissive[3];
    float glossLevel[2];
    float opacityReflectionRefractionSpectype[1];
    float luminanceRange[1];
    float fresnelParam[2];
    float sonicEyeHighLightPosition[3];
    float sonicEyeHighLightColor[3];
    float sonicSkinFalloffParam[3];
    float chrEmissionParam[4];
    float transColorMask[3];
    float emissionParam[4];
    float offsetParam[4];
    float waterParam[4];
    float furParam[4];
    float furParam2[4];
    float chrEyeFHL1[4];
    float chrEyeFHL2[4];
    float chrEyeFHL3[4];
};
