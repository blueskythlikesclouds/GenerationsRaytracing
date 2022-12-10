#include "ComputeLuminancePass.h"

#include "App.h"
#include "Math.h"

ComputeLuminancePass::ComputeLuminancePass(const App& app)
{
    bindingLayout = app.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    linearClampSampler = app.device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp));

    auto pipelineDesc = nvrhi::GraphicsPipelineDesc()
        .setVertexShader(app.shaderLibrary.copyVS)
        .setPrimType(nvrhi::PrimitiveType::TriangleList)
        .setRenderState(nvrhi::RenderState()
            .setDepthStencilState(nvrhi::DepthStencilState()
                .disableDepthTest()
                .disableDepthWrite()
                .disableStencil())
            .setRasterState(nvrhi::RasterState()
                .setCullNone()))
        .addBindingLayout(bindingLayout);

    int luminanceWidth = nextPowerOfTwo(app.renderer.raytracingPass.texture->getDesc().width);
    int luminanceHeight = nextPowerOfTwo(app.renderer.raytracingPass.texture->getDesc().height);

    while (luminanceWidth >= app.renderer.raytracingPass.texture->getDesc().width) luminanceWidth >>= 1;
    while (luminanceHeight >= app.renderer.raytracingPass.texture->getDesc().height) luminanceHeight >>= 1;

    auto previousTexture = app.renderer.raytracingPass.texture;

    while (true)
    {
        if (luminanceWidth == 0) luminanceWidth = 1;
        if (luminanceHeight == 0) luminanceHeight = 1;

        auto& pass = subPasses.emplace_back();

        pass.texture = app.device.nvrhi->createTexture(nvrhi::TextureDesc()
            .setWidth(luminanceWidth)
            .setHeight(luminanceHeight)
            .setFormat(nvrhi::Format::R16_FLOAT)
            .setInitialState(nvrhi::ResourceStates::RenderTarget)
            .setKeepInitialState(true)
            .setIsRenderTarget(true));

        pass.framebuffer = app.device.nvrhi->createFramebuffer(nvrhi::FramebufferDesc()
            .addColorAttachment(pass.texture));

        pass.pipeline = app.device.nvrhi->createGraphicsPipeline(pipelineDesc
            .setPixelShader(previousTexture == app.renderer.raytracingPass.texture ? app.shaderLibrary.copyLuminancePS : app.shaderLibrary.copyPS), pass.framebuffer);

        pass.bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, app.renderer.constantBuffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(0, previousTexture))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(1, previousTexture))
            .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), bindingLayout);

        pass.graphicsState
            .setPipeline(pass.pipeline)
            .addBindingSet(pass.bindingSet)
            .setFramebuffer(pass.framebuffer)
            .setViewport(nvrhi::ViewportState()
                .addViewportAndScissorRect(nvrhi::Viewport((float)luminanceWidth, (float)luminanceHeight)));

        previousTexture = pass.texture;

        if (luminanceWidth == 1 && luminanceHeight == 1)
            break;

        luminanceWidth >>= 1;
        luminanceHeight >>= 1;
    }

    auto textureDesc = nvrhi::TextureDesc()
        .setWidth(1)
        .setHeight(1)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setFormat(nvrhi::Format::R16_FLOAT);

    adaptedLuminanceTexture = app.device.nvrhi->createTexture(textureDesc);

    adaptedLuminanceSubPass.texture = app.device.nvrhi->createTexture(textureDesc
        .setIsRenderTarget(true));

    adaptedLuminanceSubPass.framebuffer = app.device.nvrhi->createFramebuffer(nvrhi::FramebufferDesc()
        .addColorAttachment(adaptedLuminanceSubPass.texture));

    adaptedLuminanceSubPass.pipeline = app.device.nvrhi->createGraphicsPipeline(pipelineDesc
        .setPixelShader(app.shaderLibrary.lerpLuminancePS), adaptedLuminanceSubPass.framebuffer);

    adaptedLuminanceSubPass.bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, app.renderer.constantBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, adaptedLuminanceTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, previousTexture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), bindingLayout);

    adaptedLuminanceSubPass.graphicsState
        .setPipeline(adaptedLuminanceSubPass.pipeline)
        .addBindingSet(adaptedLuminanceSubPass.bindingSet)
        .setFramebuffer(adaptedLuminanceSubPass.framebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(1.0f, 1.0f)));
}

void ComputeLuminancePass::execute(const App& app) const
{
    for (auto& subPass : subPasses)
    {
        app.renderer.commandList->setGraphicsState(subPass.graphicsState);
        app.renderer.commandList->draw(app.renderer.quadDrawArgs);
    }

    app.renderer.commandList->copyTexture(adaptedLuminanceTexture, nvrhi::TextureSlice(), adaptedLuminanceSubPass.texture, nvrhi::TextureSlice());
    app.renderer.commandList->setGraphicsState(adaptedLuminanceSubPass.graphicsState);
    app.renderer.commandList->draw(app.renderer.quadDrawArgs);
}
