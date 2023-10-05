#pragma once

struct Material
{
    struct Texture
    {
        uint32_t id;
        uint32_t samplerId;
        uint32_t texCoordIndex;
    };

    Texture diffuseTexture;
    Texture specularTexture;
    Texture powerTexture;
    Texture normalTexture;
    Texture emissionTexture;
    Texture diffuseBlendTexture;
    Texture powerBlendTexture;
    Texture normalBlendTexture;
    Texture environmentTexture;

    float diffuseColor[4];
    float specularColor[3];
    float specularPower;
    float falloffParam[3];
    uint32_t padding0;
};