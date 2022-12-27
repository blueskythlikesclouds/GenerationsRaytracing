#pragma once

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

struct BottomLevelAS
{
    nvrhi::rt::AccelStructDesc desc;
    std::vector<Geometry> geometries;
    nvrhi::rt::AccelStructHandle handle;
    std::vector<nvrhi::BufferHandle> buffers;
    std::vector<ComPtr<D3D12MA::Allocation>> allocations;
};

struct RaytracingBridge
{
    nvrhi::ShaderLibraryHandle shaderLibrary;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle geometryBindlessLayout;
    nvrhi::BindingLayoutHandle textureBindlessLayout;

    nvrhi::BufferHandle materialBuffer;
    ComPtr<D3D12MA::Allocation> materialBufferAllocation;

    nvrhi::BufferHandle geometryBuffer;
    ComPtr<D3D12MA::Allocation> geometryBufferAllocation;

    nvrhi::TextureHandle texture;
    nvrhi::TextureHandle depth;

    nvrhi::DescriptorTableHandle geometryDescriptorTable;
    nvrhi::DescriptorTableHandle textureDescriptorTable;

    nvrhi::SamplerHandle linearRepeatSampler;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    std::unordered_map<unsigned int, Material> materials;
    std::unordered_map<unsigned int, BottomLevelAS> bottomLevelAccelStructs;
    std::vector<nvrhi::rt::InstanceDesc> instanceDescs;

    float prevEyePos[3]{};

    nvrhi::SamplerHandle pointClampSampler;
    nvrhi::BindingLayoutHandle copyBindingLayout;
    nvrhi::BindingSetHandle copyBindingSet;
    nvrhi::ShaderHandle copyVertexShader;
    nvrhi::ShaderHandle copyPixelShader;
    nvrhi::FramebufferHandle copyFramebuffer;
    nvrhi::GraphicsPipelineHandle copyPipeline;
    nvrhi::GraphicsState copyGraphicsState;
    nvrhi::DrawArguments copyDrawArguments;

    void procMsgCreateGeometry(Bridge& bridge);
    void procMsgCreateBottomLevelAS(Bridge& bridge);
    void procMsgCreateInstance(Bridge& bridge);
    void procMsgCreateMaterial(Bridge& bridge);
    void procMsgNotifySceneTraversed(Bridge& bridge);
};
