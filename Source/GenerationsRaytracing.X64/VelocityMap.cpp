#include "VelocityMap.h"

#include "Bridge.h"
#include "Message.h"

#include "CopyVelocityMapPS.h"
#include "Upscaler.h"

VelocityMap::VelocityMap(const Device& device)
{
    bindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::PushConstants(0, 4))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    shader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), COPY_VELOCITY_MAP_PS, sizeof(COPY_VELOCITY_MAP_PS));
}

void VelocityMap::procMsgCopyVelocityMap(Bridge& bridge, RaytracingBridge& rtBridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCopyVelocityMap>();

    if (rtBridge.upscaler == nullptr || rtBridge.upscaler->motionVector == nullptr)
        return;

    if (!pipeline)
    {
        framebuffer = bridge.device.nvrhi->createFramebuffer(bridge.framebufferDesc);

        pipeline = bridge.device.nvrhi->createGraphicsPipeline(nvrhi::GraphicsPipelineDesc()
            .setVertexShader(rtBridge.copyVertexShader)
            .setPixelShader(shader)
            .setPrimType(nvrhi::PrimitiveType::TriangleList)
            .setRenderState(nvrhi::RenderState()
                .setDepthStencilState(nvrhi::DepthStencilState()
                    .disableDepthTest()
                    .disableDepthWrite()
                    .disableStencil())
                .setRasterState(nvrhi::RasterState()
                    .setCullNone()))
            .addBindingLayout(bindingLayout), framebuffer);
    }

    auto bindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::PushConstants(0, 4))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, rtBridge.upscaler->motionVector))
        .addItem(nvrhi::BindingSetItem::Sampler(0, rtBridge.pointClampSampler)), bindingLayout);

    bridge.commandList->setGraphicsState(nvrhi::GraphicsState()
        .setPipeline(pipeline)
        .addBindingSet(bindingSet)
        .setFramebuffer(framebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().width,
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().height))));

    bridge.commandList->setPushConstants(&msg->enableBoostBlur, 4);

    bridge.commandList->draw(rtBridge.copyDrawArguments);
}
