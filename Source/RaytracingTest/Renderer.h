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
    nvrhi::ShaderHandle shaderCopyLuminancePS;
    nvrhi::ShaderHandle shaderLerpLuminancePS;
    nvrhi::ShaderHandle shaderToneMapPS;

    nvrhi::BufferHandle constantBuffer;
    nvrhi::TextureHandle texture;

    nvrhi::SamplerHandle linearRepeatSampler;
    nvrhi::SamplerHandle linearClampSampler;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle bindlessLayout;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    nvrhi::BindingSetHandle bindingSet;
    nvrhi::DescriptorTableHandle descriptorTable;

    nvrhi::BindingLayoutHandle luminanceBindingLayout;

    struct Pass
    {
        nvrhi::TextureHandle texture;
        nvrhi::FramebufferHandle framebuffer;
        nvrhi::GraphicsPipelineHandle pipeline;
        nvrhi::BindingSetHandle bindingSet;
        nvrhi::GraphicsState graphicsState;
    };

    std::vector<Pass> luminancePasses;

    nvrhi::TextureHandle luminanceLerpedTexture;
    Pass luminanceLerpPass;

    std::vector<Pass> swapChainPasses;

    nvrhi::CommandListHandle commandList;

    int sampleCount = 0;

    Renderer(const Device& device, const Window& window, const ShaderLibrary& shaderLibraryHolder);

    void update(const App& app, float deltaTime, Scene& scene);
};
