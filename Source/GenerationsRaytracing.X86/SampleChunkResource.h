#pragma once

struct SampleChunkHeaderV1
{
    uint32_t fileSize;
    uint32_t version;
    uint32_t dataSize;
    uint32_t data;
    uint32_t relocationTable;
    uint32_t fileName;
};

struct SampleChunkHeaderV2
{
    uint32_t fileSize;
    uint32_t version;
    uint32_t relocationTable;
    uint32_t offsetCount;
};

struct SampleChunkResource
{
    static inline bool s_optimizedVertexFormat = false;
    static inline bool s_triangleTopology = false;

    static void init();
};