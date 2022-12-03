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
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;
    nvrhi::BindingSetHandle bindingSet;
    nvrhi::CommandListHandle commandList;

    Renderer(const Device& device, const Window& window);

    void update(const App& app, Scene& scene);
};
