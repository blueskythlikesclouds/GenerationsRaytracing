#pragma once

struct Material
{
    uint32_t shaderType;
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
    uint32_t displacementTexture1;
    uint32_t displacementTexture2;
    uint32_t padding0;
    uint32_t padding1;

    float texCoordOffsets[8];
    float diffuse[4];
    float ambient[4];
    float specular[4];
    float emissive[4];
    float powerGlossLevel[4];
    float opacityReflectionRefractionSpectype[4];
    float luminanceRange[4];
    float fresnelParam[4];
    float sonicEyeHighLightPosition[4];
    float sonicEyeHighLightColor[4];
    float sonicSkinFalloffParam[4];
    float chrEmissionParam[4];
    //float cloakParam[4];
    //float distortionParam[4];
    //float glassRefractionParam[4];
    //float iceParam[4];
    float emissionParam[4];
    float offsetParam[4];
    float heightParam[4];
    float waterParam[4];
};