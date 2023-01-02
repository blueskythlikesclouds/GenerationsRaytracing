#pragma once

struct Device;
class Upscaler;
struct Bridge;

struct Geometry
{
    uint32_t vertexStride;
    uint32_t normalOffset;
    uint32_t tangentOffset;
    uint32_t binormalOffset;
    uint32_t texCoordOffset;
    uint32_t colorOffset;
    uint32_t colorFormat;
    uint32_t material;
    uint32_t punchThrough;
};

struct Material
{
    char shader[256];
    uint32_t textures[16]{};
    float parameters[16][4];
};

struct Instance
{
    float transform[3][4];
    float delta[4][4];
    unsigned int bottomLevelAS;
    unsigned int instanceMask;
};

struct BottomLevelAS
{
    nvrhi::rt::AccelStructDesc desc;
    std::vector<Geometry> geometries;
    nvrhi::rt::AccelStructHandle handle;
    std::vector<nvrhi::BufferHandle> buffers;
    uint32_t index = 0;

    BottomLevelAS();
};

struct RTConstants
{
    float prevProj[4][4];
    float prevView[4][4];

    float jitterX = 0.0f;
    float jitterY = 0.0f;

    int currentFrame = 0;
};

struct RaytracingBridge
{
    nvrhi::ShaderLibraryHandle shaderLibrary;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle geometryBindlessLayout;
    nvrhi::BindingLayoutHandle textureBindlessLayout;

    RTConstants rtConstants{};
    nvrhi::BufferHandle rtConstantBuffer;

    nvrhi::BufferHandle materialBuffer;
    nvrhi::BufferHandle geometryBuffer;
    nvrhi::BufferHandle instanceBuffer;

    nvrhi::TextureHandle output;
    std::unique_ptr<Upscaler> upscaler;

    nvrhi::DescriptorTableHandle geometryDescriptorTable;
    nvrhi::DescriptorTableHandle textureDescriptorTable;

    nvrhi::SamplerHandle linearRepeatSampler;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    std::unordered_map<unsigned int, Material> materials;
    std::unordered_map<unsigned int, BottomLevelAS> bottomLevelAccelStructs;
    std::vector<Instance> instances;

    nvrhi::SamplerHandle pointClampSampler;
    nvrhi::BindingLayoutHandle copyBindingLayout;
    nvrhi::ShaderHandle copyVertexShader;
    nvrhi::ShaderHandle copyPixelShader;
    nvrhi::FramebufferHandle copyFramebuffer;
    nvrhi::GraphicsPipelineHandle copyPipeline;
    nvrhi::DrawArguments copyDrawArguments;

    RaytracingBridge(const Device& device, const std::string& directoryPath);
    ~RaytracingBridge();

    void procMsgCreateGeometry(Bridge& bridge);
    void procMsgCreateBottomLevelAS(Bridge& bridge);
    void procMsgCreateInstance(Bridge& bridge);
    void procMsgCreateMaterial(Bridge& bridge);
    void procMsgNotifySceneTraversed(Bridge& bridge);
};
