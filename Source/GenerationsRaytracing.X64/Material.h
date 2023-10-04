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
    float specularColor[4];
    float specularPower;
    uint32_t padding0;
    uint32_t padding1;
    uint32_t padding2;
};