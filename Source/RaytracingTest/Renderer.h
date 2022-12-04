#pragma once

struct App;
struct Window;
struct Scene;
struct Device;

struct Renderer
{
    nvrhi::ShaderLibraryHandle shaderLibrary;

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

    Renderer(const Device& device, const Window& window);

    void update(const App& app, Scene& scene);
};
