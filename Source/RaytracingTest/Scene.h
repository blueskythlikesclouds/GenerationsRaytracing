#pragma once
#include "SceneEffect.h"

struct ShaderMapping;
struct Device;

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
    enum class Type : uint32_t
    {
        Opaque,
        Trans,
        Punch,
        Special
    };

    Type type = Type::Opaque;
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
        Type type = Type::Opaque;
    };
};

struct Model
{
    uint32_t meshOffset = 0;
    uint32_t meshCount = 0;
    uint32_t instanceMask = 1;
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
        std::vector<Eigen::Vector3f> normals;
        std::vector<Eigen::Vector4f> tangents;
        std::vector<Eigen::Vector2f> texCoords;
        std::vector<Eigen::Vector4f> colors;
        std::vector<uint16_t> indices;

        Eigen::Vector4f lightDirection;
        Eigen::Vector4f lightColor;

        SceneEffect effect;
    } cpu;

    struct GPU
    {
        std::vector<nvrhi::TextureHandle> textures;

        std::vector<nvrhi::rt::AccelStructHandle> bottomLevelAccelStructs;
        nvrhi::rt::AccelStructHandle topLevelAccelStruct;

        nvrhi::BufferHandle materialBuffer;
        nvrhi::BufferHandle meshBuffer;
        nvrhi::BufferHandle vertexBuffer;
        nvrhi::BufferHandle normalBuffer;
        nvrhi::BufferHandle tangentBuffer;
        nvrhi::BufferHandle texCoordBuffer;
        nvrhi::BufferHandle colorBuffer;
        nvrhi::BufferHandle indexBuffer;
    } gpu;

    void loadCpuResources(const std::string& directoryPath);
    void createGpuResources(const Device& device, const ShaderMapping& shaderMapping);
};