#pragma once

struct Material
{
    char shader[256]{};

    struct GPU
    {
        uint32_t textures[16]{};
        float parameters[16][4]{};
    } gpu{};
};
