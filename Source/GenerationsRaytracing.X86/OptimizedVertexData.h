#pragma once

struct OptimizedVertexData
{
    Sonic::CNoAlignVector position;
    uint32_t color;
    uint32_t normal;
    uint32_t tangent;
    uint32_t binormal;
    uint16_t texCoord[2];
};

static_assert(sizeof(OptimizedVertexData) == 0x20);