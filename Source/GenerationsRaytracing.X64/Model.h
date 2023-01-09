#pragma once

#define GEOMETRY_SKINNING_BUFFER_ID(vertexBuffer, element) (((size_t)(vertexBuffer) << 32) | (size_t)(element))
#define GEOMETRY_PREV_SKINNING_BUFFER_ID(vertexBuffer, element) (GEOMETRY_SKINNING_BUFFER_ID(vertexBuffer, element) + 1)

struct DescriptorTableAllocator;

struct Mesh
{
    struct GPU
    {
        uint32_t vertexBuffer = 0;
        uint32_t prevVertexBuffer = 0;
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
        uint32_t indexBuffer = 0;
        uint32_t material = 0;
        uint32_t punchThrough = 0;
    } gpu{};

    nvrhi::BufferHandle vertexBuffer;
    nvrhi::BufferHandle indexBuffer;
    nvrhi::BufferHandle skinningBuffer;
    nvrhi::BufferHandle prevSkinningBuffer;
    nvrhi::BufferHandle nodeIndicesBuffer;

    DescriptorTableAllocator& allocator;
    size_t skinningBufferId = 0;
    size_t prevSkinningBufferId = 0;

    Mesh(DescriptorTableAllocator& allocator);
    ~Mesh();

    void releaseDescriptors();
};

struct Group
{
    nvrhi::rt::AccelStructDesc desc;
    nvrhi::rt::AccelStructHandle handle;

    std::vector<Mesh> meshes;
    uint32_t curMeshIndex = 0;

    uint32_t indexInBuffer = 0;
};

struct Element
{
    emhash8::HashMap<unsigned int, Group> groups;

    nvrhi::BufferHandle matrixBuffer;
    bool matrixBufferChanged = false;
    XXH64_hash_t matrixHash = 0;
};

struct Model
{
    emhash8::HashMap<unsigned int, Element> elements;
    emhash8::HashMap<XXH64_hash_t, nvrhi::BufferHandle> nodeIndicesBuffers;
    emhash8::HashMap<unsigned int, nvrhi::rt::AccelStructHandle> accelStructs;
};
