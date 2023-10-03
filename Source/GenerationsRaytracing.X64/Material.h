#pragma once

struct Material
{
    struct Texture
    {
        uint32_t id;
        uint32_t samplerId;
    };

    Texture diffuseTexture;
};