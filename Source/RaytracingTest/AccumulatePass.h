#pragma once

struct App;

struct AccumulatePass
{
    nvrhi::TextureHandle previousTexture;
    nvrhi::TextureHandle texture;
    nvrhi::FramebufferHandle framebuffer;
    nvrhi::SamplerHandle linearClampSampler;
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingSetHandle bindingSet;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsState graphicsState;

    AccumulatePass(const App& app);
    void execute(const App& app) const;
};
