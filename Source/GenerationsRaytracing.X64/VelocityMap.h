#pragma once

struct Device;
struct RaytracingBridge;
struct Bridge;

struct VelocityMap
{
    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::ShaderHandle shader;
    nvrhi::FramebufferHandle framebuffer;
    nvrhi::GraphicsPipelineHandle pipeline;

    VelocityMap(const Device& device);

    void procMsgCopyVelocityMap(Bridge& bridge, RaytracingBridge& rtBridge);
};
