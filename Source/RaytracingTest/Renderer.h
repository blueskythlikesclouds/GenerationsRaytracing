#pragma once

struct ShaderLibrary;
struct App;
struct Window;
struct Scene;
struct Device;

struct Renderer
{
    nvrhi::ShaderLibraryHandle shaderLibrary0;
    nvrhi::ShaderLibraryHandle shaderLibrary1;

    nvrhi::BufferHandle constantBuffer;
    nvrhi::TextureHandle texture;
    nvrhi::SamplerHandle sampler;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle bindlessLayout;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    nvrhi::BindingSetHandle bindingSet;
    nvrhi::DescriptorTableHandle descriptorTable;

    nvrhi::CommandListHandle commandList;

    int sampleCount = 0;

    Renderer(const Device& device, const Window& window, const ShaderLibrary& shaderLibrary);

    void update(const App& app, Scene& scene);
};
