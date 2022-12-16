#include "AccumulatePass.h"
#include "App.h"

AccumulatePass::AccumulatePass(const App& app)
{
    auto textureDesc = nvrhi::TextureDesc()
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setWidth(app.width)
        .setHeight(app.height)
        .setKeepInitialState(true);

    previousTexture = app.device.nvrhi->createTexture(textureDesc
        .setInitialState(nvrhi::ResourceStates::ShaderResource));

    texture = app.device.nvrhi->createTexture(textureDesc
        .setIsRenderTarget(true)
        .setInitialState(nvrhi::ResourceStates::RenderTarget));

    framebuffer = app.device.nvrhi->createFramebuffer(nvrhi::FramebufferDesc()
        .addColorAttachment(texture));

    linearClampSampler = app.device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp));

    bindingLayout = app.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, app.renderer.constantBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, previousTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, app.renderer.dlssPass.outputTexture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), bindingLayout);

    pipeline = app.device.nvrhi->createGraphicsPipeline(nvrhi::GraphicsPipelineDesc()
        .setVertexShader(app.shaderLibrary.copyVS)
        .setPixelShader(app.shaderLibrary.accumulatePS)
        .setPrimType(nvrhi::PrimitiveType::TriangleList)
        .setRenderState(nvrhi::RenderState()
            .setDepthStencilState(nvrhi::DepthStencilState()
                .disableDepthTest()
                .disableDepthWrite()
                .disableStencil())
            .setRasterState(nvrhi::RasterState()
                .setCullNone()))
        .addBindingLayout(bindingLayout), framebuffer);

    graphicsState
        .setPipeline(pipeline)
        .addBindingSet(bindingSet)
        .setFramebuffer(framebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().width,
                (float)framebuffer->getDesc().colorAttachments[0].texture->getDesc().height)));
}

void AccumulatePass::execute(const App& app) const
{
    if (app.camera.currentFrame > 1)
    {
        app.renderer.commandList->copyTexture(previousTexture, nvrhi::TextureSlice(), texture, nvrhi::TextureSlice());
        app.renderer.commandList->setGraphicsState(graphicsState);
        app.renderer.commandList->draw(app.renderer.quadDrawArgs);
    }
    else
    {
        app.renderer.commandList->copyTexture(texture, nvrhi::TextureSlice(), app.renderer.dlssPass.outputTexture, nvrhi::TextureSlice());
    }
}
