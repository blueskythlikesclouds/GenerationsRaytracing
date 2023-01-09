#pragma once

#include "ShaderMapping.h"

struct Material
{
    size_t shader = 0;

    struct GPU
    {
        uint32_t textures[16]{};
        float parameters[16][4]{};
    } gpu{};

    uint32_t indexInBuffer = 0;
};
