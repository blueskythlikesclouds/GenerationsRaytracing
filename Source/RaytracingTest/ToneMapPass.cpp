#include "ToneMapPass.h"
#include "App.h"

ToneMapPass::ToneMapPass(const App& app, nvrhi::IFramebuffer* framebuffer)
{
    bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, app.renderer.constantBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, app.renderer.dlssPass.outputTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, app.renderer.computeLuminancePass.adaptedLuminanceSubPass.texture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, app.renderer.computeLuminancePass.linearClampSampler)), app.renderer.computeLuminancePass.bindingLayout);

    pipeline = app.device.nvrhi->createGraphicsPipeline(nvrhi::GraphicsPipelineDesc()
        .setVertexShader(app.shaderLibrary.copyVS)
        .setPixelShader(app.shaderLibrary.toneMapPS)
        .setPrimType(nvrhi::PrimitiveType::TriangleList)
        .setRenderState(nvrhi::RenderState()
            .setDepthStencilState(nvrhi::DepthStencilState()
                .disableDepthTest()
                .disableDepthWrite()
                .disableStencil())
            .setRasterState(nvrhi::RasterState()
                .setCullNone()))
        .addBindingLayout(app.renderer.computeLuminancePass.bindingLayout), framebuffer);

    graphicsState
        .setPipeline(pipeline)
        .addBindingSet(bindingSet)
        .setFramebuffer(framebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().width, 
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().height)));
}

void ToneMapPass::execute(const App& app) const
{
    app.renderer.commandList->setGraphicsState(graphicsState);
    app.renderer.commandList->draw(app.renderer.quadDrawArgs);
}
