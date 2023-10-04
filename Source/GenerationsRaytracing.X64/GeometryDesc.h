#pragma once

struct GeometryDesc
{
    uint32_t indexBufferId;
    uint32_t vertexBufferId;
    uint32_t vertexStride;
    uint32_t normalOffset;
    uint32_t tangentOffset;
    uint32_t binormalOffset;
    uint32_t texCoordOffset;
    uint32_t colorOffset;
    uint32_t materialId;
};