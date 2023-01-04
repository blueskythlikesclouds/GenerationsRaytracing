#pragma once

struct BottomLevelAS
{
    struct Geometry
    {
        struct GPU
        {
            uint32_t vertexCount = 0;
            uint32_t vertexStride = 0;
            uint32_t normalOffset = 0;
            uint32_t tangentOffset = 0;
            uint32_t binormalOffset = 0;
            uint32_t texCoordOffset = 0;
            uint32_t colorOffset = 0;
            uint32_t colorFormat = 0;
            uint32_t blendWeightOffset = 0;
            uint32_t blendIndicesOffset = 0;
            uint32_t material = 0;
            uint32_t punchThrough = 0;
        } gpu{};

        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BufferHandle skinningBuffer;
        nvrhi::BufferHandle prevSkinningBuffer;
        nvrhi::BufferHandle nodeIndicesBuffer;
    };

    struct Instance
    {
        nvrhi::rt::AccelStructDesc desc;
        nvrhi::rt::AccelStructHandle handle;

        std::vector<Geometry> geometries;
        uint32_t curGeometryIndex = 0;

        nvrhi::BufferHandle matrixBuffer;
        XXH64_hash_t matrixHash = 0;

        uint32_t indexInBuffer = 0;
    };

    std::unordered_map<unsigned int, Instance> instances;
    std::unordered_map<XXH64_hash_t, nvrhi::BufferHandle> nodeIndicesBuffers;
};
