#pragma once

struct ShaderLibrary;
struct App;
struct Window;
struct Scene;
struct Device;

struct Renderer
{
	nvrhi::ShaderLibraryHandle shaderLibrary;
    nvrhi::ShaderHandle shaderCopyVS;
    nvrhi::ShaderHandle shaderCopyPS;

    nvrhi::BufferHandle constantBuffer;
    nvrhi::TextureHandle texture;
    nvrhi::SamplerHandle sampler;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle bindlessLayout;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    nvrhi::BindingSetHandle bindingSet;
    nvrhi::DescriptorTableHandle descriptorTable;

    std::vector<nvrhi::GraphicsPipelineHandle> graphicsPipelines;

    nvrhi::CommandListHandle commandList;

    int sampleCount = 0;

    Renderer(const Device& device, const Window& window, const ShaderLibrary& shaderLibraryHolder);

    void update(const App& app, Scene& scene);
};
