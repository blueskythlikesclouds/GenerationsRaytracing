#pragma once

struct App;
struct Scene;

struct RaytracingPass
{
    nvrhi::TextureHandle texture;
    nvrhi::SamplerHandle linearRepeatSampler;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingLayoutHandle bindlessLayout;

    nvrhi::rt::PipelineHandle pipeline;
    nvrhi::rt::ShaderTableHandle shaderTable;

    nvrhi::BindingSetHandle bindingSet;
    nvrhi::DescriptorTableHandle descriptorTable;

    RaytracingPass(const App& app);
    void execute(const App& app, Scene& scene);
};
