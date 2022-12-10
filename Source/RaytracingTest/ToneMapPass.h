#pragma once

struct App;

struct ToneMapPass
{
    nvrhi::BindingSetHandle bindingSet;
    nvrhi::GraphicsPipelineHandle pipeline;
    nvrhi::GraphicsState graphicsState;

    ToneMapPass(const App& app, nvrhi::IFramebuffer* framebuffer);
    void execute(const App& app) const;
};
