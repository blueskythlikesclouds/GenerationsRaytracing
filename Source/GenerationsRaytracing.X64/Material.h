#pragma once

struct Material
{
    uint32_t shaderType : 16;
    uint32_t flags : 16;
    float texCoordOffsets[8];
    uint32_t packedData[35];
};
