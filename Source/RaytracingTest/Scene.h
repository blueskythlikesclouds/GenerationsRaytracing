#pragma once

#include "SceneEffect.h"

struct App;

struct Texture
{
    std::unique_ptr<uint8_t[]> data;
    size_t dataSize = 0;
};

struct Material
{
    std::string shader;
    std::vector<std::pair<std::string, Eigen::Vector4f>> parameters;
    std::vector<std::pair<std::string, uint32_t>> textures;

    struct GPU
    {
        Eigen::Vector4f parameters[16]{};
        uint32_t textureIndices[16]{};
    };
};

struct Mesh
{
    uint32_t type = 0;
    uint32_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;

    struct GPU
    {
        uint32_t vertexOffset;
        uint32_t indexOffset;
        uint32_t materialIndex;
        uint32_t type;
    };
};

struct Model
{
    uint32_t meshOffset = 0;
    uint32_t meshCount = 0;
    uint32_t instanceMask = 0;
};

struct Instance
{
    Eigen::Matrix4f transform;
    uint32_t modelIndex = 0;

    Instance()
        : transform(Eigen::Matrix4f::Identity())
    {
        
    }
};

struct Light
{
    Eigen::Vector3f position;
    float outerRadius;
    Eigen::Vector3f color;
    float innerRadius;
};

struct Scene
{
    struct CPU
    {
        std::vector<Texture> textures;
        std::vector<Material> materials;

        std::vector<Mesh> meshes;
        std::vector<Model> models;
        std::vector<Instance> instances;

        std::vector<Eigen::Vector3f> vertices;
        std::vector<uint32_t> normals;
        std::vector<uint32_t> tangents;
        std::vector<uint32_t> texCoords;
        std::vector<uint32_t> colors;
        std::vector<uint16_t> indices;

        Light globalLight;
        std::vector<Light> localLights;

        SceneEffect effect;
    } cpu;

    struct GPU
    {
        std::vector<nvrhi::TextureHandle> textures;
        std::vector<ComPtr<D3D12MA::Allocation>> allocations;

        std::vector<nvrhi::rt::AccelStructHandle> modelBVHs;
        nvrhi::rt::AccelStructHandle bvh;
        nvrhi::rt::AccelStructHandle shadowBVH;
        nvrhi::rt::AccelStructHandle skyBVH;

        nvrhi::BufferHandle materialBuffer;
        nvrhi::BufferHandle meshBuffer;
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle normalBuffer;
        nvrhi::BufferHandle tangentBuffer;
        nvrhi::BufferHandle texCoordBuffer;
        nvrhi::BufferHandle colorBuffer;
        nvrhi::BufferHandle indexBuffer;
        nvrhi::BufferHandle lightBuffer;
    } gpu;

    void loadCpuResources(const std::string& directoryPath);
    void createGpuResources(const App& app);
};