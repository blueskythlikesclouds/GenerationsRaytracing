#pragma once

struct Bridge;

struct Raytracing
{
    nvrhi::ShaderLibraryHandle shaderLibrary;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::TextureHandle texture;
    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    std::unordered_map<unsigned int, nvrhi::rt::AccelStructDesc> blasDescs;
    std::unordered_map<unsigned int, nvrhi::rt::AccelStructHandle> bottomLevelAccelStructs;
    std::vector<nvrhi::rt::InstanceDesc> instanceDescs;

    void procMsgCreateGeometry(Bridge& bridge);
    void procMsgCreateBottomLevelAS(Bridge& bridge);
    void procMsgCreateInstance(Bridge& bridge);
    void procMsgNotifySceneTraversed(Bridge& bridge);
};
