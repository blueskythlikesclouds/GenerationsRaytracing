#pragma once

struct App;

struct ComputeLuminancePass
{
    struct SubPass
    {
        nvrhi::TextureHandle texture;
        nvrhi::FramebufferHandle framebuffer;
        nvrhi::GraphicsPipelineHandle pipeline;
        nvrhi::BindingSetHandle bindingSet;
        nvrhi::GraphicsState graphicsState;
    };

    nvrhi::TextureHandle adaptedLuminanceTexture;

    std::vector<SubPass> subPasses;
    SubPass adaptedLuminanceSubPass;

    ComputeLuminancePass(const App& app);
    void execute(const App& app) const;
};
