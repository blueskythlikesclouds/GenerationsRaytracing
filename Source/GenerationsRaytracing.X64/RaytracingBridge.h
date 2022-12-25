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
};

struct BottomLevelAS
{
    nvrhi::rt::AccelStructDesc desc;
    std::vector<Geometry> geometries;
    nvrhi::rt::AccelStructHandle handle;
};

struct RaytracingBridge
{
    nvrhi::ShaderLibraryHandle shaderLibrary;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle bindlessLayout;

    nvrhi::BufferHandle geometryBuffer;
    ComPtr<D3D12MA::Allocation> geometryBufferAllocation;
    nvrhi::TextureHandle texture;
    nvrhi::DescriptorTableHandle descriptorTable;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    std::unordered_map<unsigned int, BottomLevelAS> bottomLevelAccelStructs;
    std::vector<nvrhi::rt::InstanceDesc> instanceDescs;

    void procMsgCreateGeometry(Bridge& bridge);
    void procMsgCreateBottomLevelAS(Bridge& bridge);
    void procMsgCreateInstance(Bridge& bridge);
    void procMsgNotifySceneTraversed(Bridge& bridge);
};
