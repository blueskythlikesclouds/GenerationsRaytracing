#include "Renderer.h"

#include "App.h"
#include "Math.h"
#include "Scene.h"
#include "ShaderCopyLuminancePS.h"
#include "ShaderCopyPS.h"
#include "ShaderCopyVS.h"
#include "ShaderLerpLuminancePS.h"
#include "ShaderToneMapPS.h"

struct ConstantBuffer
{
    Eigen::Vector3f position;
    float tanFovy;
    Eigen::Matrix4f rotation;

    float aspectRatio;
    uint32_t sampleCount;
    float skyIntensityScale;
    float deltaTime;

    Eigen::Vector4f lightDirection;
    Eigen::Vector4f lightColor;

    Eigen::Vector4f lightScatteringColor;
    Eigen::Vector4f rayMieRay2Mie2;
    Eigen::Vector4f gAndFogDensity;
    Eigen::Vector4f farNearScale;

    Eigen::Vector4f eyeLightDiffuse;
    Eigen::Vector4f eyeLightSpecular;
    Eigen::Vector4f eyeLightRange;
    Eigen::Vector4f eyeLightAttribute;

    float middleGray;
    float lumMin;
    float lumMax;
    uint32_t lightCount;
};

Renderer::Renderer(const Device& device, const Window& window, const ShaderLibrary& shaderLibraryHolder)
{
    shaderLibrary = device.nvrhi->createShaderLibrary(shaderLibraryHolder.byteCode.get(), shaderLibraryHolder.byteSize);

    shaderCopyVS = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), SHADER_COPY_VS, sizeof(SHADER_COPY_VS));
    shaderCopyPS = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_COPY_PS, sizeof(SHADER_COPY_PS));
    shaderLerpLuminancePS = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_LERP_LUMINANCE_PS, sizeof(SHADER_LERP_LUMINANCE_PS));
    shaderCopyLuminancePS = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_COPY_LUMINANCE_PS, sizeof(SHADER_COPY_LUMINANCE_PS));
    shaderToneMapPS = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_TONE_MAP_PS, sizeof(SHADER_TONE_MAP_PS));

    constantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(ConstantBuffer))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    auto textureDesc = window.getCurrentSwapChainTexture()->getDesc();
    texture = device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setIsUAV(true)
        .setIsRenderTarget(false)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true));

    linearRepeatSampler = device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat));

    linearClampSampler = device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Clamp));

    bindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))
        .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(1))
        .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(2))
        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(3))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(4))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(5))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(6))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(7))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(8))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(9))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(10))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    bindlessLayout = device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .setMaxCapacity(2048)
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(2)));

    auto pipelineDesc = nvrhi::rt::PipelineDesc()
        .addBindingLayout(bindingLayout)
        .addBindingLayout(bindlessLayout)
        .addShader({ "", shaderLibrary->getShader("RayGeneration", nvrhi::ShaderType::RayGeneration), nullptr })
        .addShader({ "", shaderLibrary->getShader("Miss", nvrhi::ShaderType::Miss), nullptr })
        .setMaxRecursionDepth(8)
        .setMaxPayloadSize(32);

    for (auto& shaderPair : shaderLibraryHolder.shaderMapping.map)
    {
        pipelineDesc.addHitGroup(
        {
            shaderPair.second.name,
            shaderLibrary->getShader(shaderPair.second.closestHit.c_str(), nvrhi::ShaderType::ClosestHit),
            shaderLibrary->getShader(shaderPair.second.anyHit.c_str(), nvrhi::ShaderType::AnyHit)
        });
    }

    pipeline = device.nvrhi->createRayTracingPipeline(pipelineDesc);

    luminanceBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    auto lumPipelineDesc = nvrhi::GraphicsPipelineDesc()
        .setVertexShader(shaderCopyVS)
        .setPrimType(nvrhi::PrimitiveType::TriangleList)
        .setRenderState(nvrhi::RenderState()
            .setDepthStencilState(nvrhi::DepthStencilState()
                .disableDepthTest()
                .disableDepthWrite()
                .disableStencil())
            .setRasterState(nvrhi::RasterState()
                .setCullNone()))
        .addBindingLayout(luminanceBindingLayout);

    int luminanceWidth = nextPowerOfTwo(window.width);
    int luminanceHeight = nextPowerOfTwo(window.height);

    while (luminanceWidth >= window.width) luminanceWidth >>= 1;
    while (luminanceHeight >= window.height) luminanceHeight >>= 1;

    auto previousTexture = texture;

    while (true)
    {
        if (luminanceWidth == 0) luminanceWidth = 1;
        if (luminanceHeight == 0) luminanceHeight = 1;

        auto& pass = luminancePasses.emplace_back();

        pass.texture = device.nvrhi->createTexture(nvrhi::TextureDesc()
            .setWidth(luminanceWidth)
            .setHeight(luminanceHeight)
            .setFormat(nvrhi::Format::R16_FLOAT)
            .setInitialState(nvrhi::ResourceStates::RenderTarget)
            .setKeepInitialState(true)
            .setIsRenderTarget(true));

        pass.framebuffer = device.nvrhi->createFramebuffer(nvrhi::FramebufferDesc()
            .addColorAttachment(pass.texture));

        pass.pipeline = device.nvrhi->createGraphicsPipeline(lumPipelineDesc
            .setPixelShader(previousTexture == texture ? shaderCopyLuminancePS : shaderCopyPS), pass.framebuffer);

        pass.bindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constantBuffer))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(0, previousTexture))
            .addItem(nvrhi::BindingSetItem::Texture_SRV(1, previousTexture))
            .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), luminanceBindingLayout);

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

    auto luminanceLerpedTextureDesc = nvrhi::TextureDesc()
        .setWidth(1)
        .setHeight(1)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setFormat(nvrhi::Format::R16_FLOAT);

    luminanceLerpedTexture = device.nvrhi->createTexture(luminanceLerpedTextureDesc);

    luminanceLerpPass.texture = device.nvrhi->createTexture(luminanceLerpedTextureDesc
        .setIsRenderTarget(true));

    luminanceLerpPass.framebuffer = device.nvrhi->createFramebuffer(nvrhi::FramebufferDesc()
        .addColorAttachment(luminanceLerpPass.texture));

    luminanceLerpPass.pipeline = device.nvrhi->createGraphicsPipeline(lumPipelineDesc
        .setPixelShader(shaderLerpLuminancePS), luminanceLerpPass.framebuffer);

    luminanceLerpPass.bindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constantBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, luminanceLerpedTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, previousTexture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), luminanceBindingLayout);

    luminanceLerpPass.graphicsState
        .setPipeline(luminanceLerpPass.pipeline)
        .addBindingSet(luminanceLerpPass.bindingSet)
        .setFramebuffer(luminanceLerpPass.framebuffer)
        .setViewport(nvrhi::ViewportState()
            .addViewportAndScissorRect(nvrhi::Viewport(1.0f, 1.0f)));

    auto swapChainBindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constantBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, texture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, luminanceLerpPass.texture))
        .addItem(nvrhi::BindingSetItem::Sampler(0, linearClampSampler)), luminanceBindingLayout);

    for (auto& swapChainFramebuffer : window.nvrhi.swapChainFramebuffers)
    {
        auto& pass = swapChainPasses.emplace_back();

        pass.bindingSet = swapChainBindingSet;

        pass.pipeline = device.nvrhi->createGraphicsPipeline(lumPipelineDesc
            .setPixelShader(shaderToneMapPS), swapChainFramebuffer);

        pass.graphicsState
            .setPipeline(pass.pipeline)
            .addBindingSet(swapChainBindingSet)
            .setFramebuffer(swapChainFramebuffer)
            .setViewport(nvrhi::ViewportState()
                .addViewportAndScissorRect(nvrhi::Viewport((float)window.width, (float)window.height)));
    }

    commandList = device.nvrhi->createCommandList();
}

void Renderer::update(const App& app, float deltaTime, Scene& scene)
{
    if (!scene.gpu.bvh)
        scene.createGpuResources(app.device, app.shaderLibrary.shaderMapping);

    assert(scene.gpu.bvh);

    if (!bindingSet)
    {
        bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constantBuffer))
            .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, scene.gpu.bvh))
            .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(1, scene.gpu.shadowBVH))
            .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(2, scene.gpu.skyBVH))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(3, scene.gpu.materialBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(4, scene.gpu.meshBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(5, scene.gpu.normalBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(6, scene.gpu.tangentBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(7, scene.gpu.texCoordBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(8, scene.gpu.colorBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(9, scene.gpu.indexBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(10, scene.gpu.lightBuffer))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(0, texture))
            .addItem(nvrhi::BindingSetItem::Sampler(0, linearRepeatSampler)), bindingLayout);

        shaderTable = pipeline->createShaderTable();
        shaderTable->setRayGenerationShader("RayGeneration");
        shaderTable->addMissShader("Miss");

        for (auto& mesh : scene.cpu.meshes)
            shaderTable->addHitGroup(scene.cpu.materials[mesh.materialIndex].shader.c_str());

        descriptorTable = app.device.nvrhi->createDescriptorTable(bindlessLayout);

        app.device.nvrhi->resizeDescriptorTable(descriptorTable, (uint32_t)scene.gpu.textures.size(), false);
        for (size_t i = 0; i < scene.gpu.textures.size(); i++)
            app.device.nvrhi->writeDescriptorTable(descriptorTable, nvrhi::BindingSetItem::Texture_SRV((uint32_t)i, scene.gpu.textures[i]));
    }

    if (app.camera.dirty)
        sampleCount = 0;

    commandList->open();

    ConstantBuffer bufferData {};

    const float fieldOfView = app.camera.fieldOfView / 180.0f * (float)M_PI;
    const float aspectRatio = (float)app.window.width / (float)app.window.height;

    bufferData.position = app.camera.position;
    bufferData.tanFovy = tan(fieldOfView / 2.0f);
    bufferData.rotation.block(0, 0, 3, 3) = app.camera.rotation.toRotationMatrix();

    bufferData.aspectRatio = aspectRatio;
    bufferData.sampleCount = ++sampleCount;
    bufferData.skyIntensityScale = scene.cpu.effect.defaults.skyIntensityScale;
    bufferData.deltaTime = deltaTime;

    bufferData.lightDirection.head<3>() = scene.cpu.globalLight.position;
    bufferData.lightColor.head<3>() = scene.cpu.globalLight.color;

    bufferData.lightScatteringColor = scene.cpu.effect.lightScattering.color;
    bufferData.rayMieRay2Mie2 = scene.cpu.effect.lightScattering.gpu.rayMieRay2Mie2;
    bufferData.gAndFogDensity = scene.cpu.effect.lightScattering.gpu.gAndFogDensity;
    bufferData.farNearScale = scene.cpu.effect.lightScattering.gpu.farNearScale;

    bufferData.eyeLightDiffuse = scene.cpu.effect.eyeLight.diffuse;
    bufferData.eyeLightSpecular = scene.cpu.effect.eyeLight.specular;
    bufferData.eyeLightRange = scene.cpu.effect.eyeLight.gpu.range;
    bufferData.eyeLightAttribute = scene.cpu.effect.eyeLight.gpu.attribute;

    bufferData.middleGray = scene.cpu.effect.hdr.middleGray;
    bufferData.lumMin = scene.cpu.effect.hdr.lumMin;
    bufferData.lumMax = scene.cpu.effect.hdr.lumMax;

    bufferData.lightCount = (uint32_t)scene.cpu.localLights.size();

    commandList->writeBuffer(constantBuffer, &bufferData, sizeof(ConstantBuffer));

    commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
        .addBindingSet(descriptorTable));

    commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(app.window.width)
        .setHeight(app.window.height));

    auto drawArgs = nvrhi::DrawArguments()
        .setVertexCount(6);

    for (auto& luminancePass : luminancePasses)
    {
        commandList->setGraphicsState(luminancePass.graphicsState);
        commandList->draw(drawArgs);
    }

    commandList->copyTexture(luminanceLerpedTexture, nvrhi::TextureSlice(), luminanceLerpPass.texture, nvrhi::TextureSlice());
    commandList->setGraphicsState(luminanceLerpPass.graphicsState);
    commandList->draw(drawArgs);

    commandList->setGraphicsState(swapChainPasses[app.window.dxgi.swapChain->GetCurrentBackBufferIndex()].graphicsState);
    commandList->draw(drawArgs);

    commandList->close();

    app.device.nvrhi->executeCommandList(commandList);
}
