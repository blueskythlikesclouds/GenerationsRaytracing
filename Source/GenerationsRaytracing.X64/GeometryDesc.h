#pragma once

struct GeometryDesc
{
    uint32_t flags : 12;
    uint32_t indexBufferId : 20;
    uint32_t indexOffset;

    uint32_t vertexBufferId : 20;
    uint32_t vertexStride : 12;
    uint32_t vertexCount;
    uint32_t vertexOffset;

    uint32_t normalOffset : 8;
    uint32_t tangentOffset : 8;
    uint32_t binormalOffset : 8;
    uint32_t colorOffset : 8;

    uint32_t texCoordOffset0 : 8;
    uint32_t texCoordOffset1 : 8;
    uint32_t texCoordOffset2 : 8;
    uint32_t texCoordOffset3 : 8;

    uint32_t materialId;
};