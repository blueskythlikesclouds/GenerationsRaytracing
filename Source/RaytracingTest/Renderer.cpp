#include "Renderer.h"

#include "App.h"
#include "Scene.h"
#include "ShaderBytecode.h"

struct ConstantBuffer
{
    Eigen::Vector3f position = Eigen::Vector3f::Zero();
    float tanFovy = 0.0f;
    Eigen::Matrix4f rotation = Eigen::Matrix4f::Identity();
    float aspectRatio = 0.0f;
};

Renderer::Renderer(const Device& device, const Window& window)
{
    shaderLibrary = device.nvrhi->createShaderLibrary(SHADER_BYTECODE, sizeof(SHADER_BYTECODE));
    constantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(ConstantBuffer))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    auto textureDesc = window.getCurrentSwapChainTexture()->getDesc();
    texture = device.nvrhi->createTexture(textureDesc
        .setIsUAV(true)
        .setIsRenderTarget(false)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true));

    sampler = device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat));

    bindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))
        .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(2))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(3))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(4))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(5))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(6))
        .addItem(nvrhi::BindingLayoutItem::TypedBuffer_SRV(7))
        .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0))
		.addItem(nvrhi::BindingLayoutItem::Sampler(0)));

    bindlessLayout = device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .setMaxCapacity(2048)
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(1)));

    pipeline = device.nvrhi->createRayTracingPipeline(nvrhi::rt::PipelineDesc()
        .addBindingLayout(bindingLayout)
        .addBindingLayout(bindlessLayout)
        .addShader({ "", shaderLibrary->getShader("RayGeneration", nvrhi::ShaderType::RayGeneration), nullptr })
        .addShader({ "", shaderLibrary->getShader("Miss", nvrhi::ShaderType::Miss), nullptr })
        .addHitGroup({ "HitGroup", shaderLibrary->getShader("ClosestHit", nvrhi::ShaderType::ClosestHit), nullptr, nullptr, nullptr, false })
        .setMaxRecursionDepth(2)
        .setMaxPayloadSize(32));

    shaderTable = pipeline->createShaderTable();
    shaderTable->setRayGenerationShader("RayGeneration");
    shaderTable->addMissShader("Miss");
    shaderTable->addHitGroup("HitGroup");

    commandList = device.nvrhi->createCommandList();
}

void Renderer::update(const App& app, Scene& scene)
{
    if (!scene.gpu.topLevelAccelStruct)
        scene.createGpuResources(app.device);

    assert(scene.gpu.topLevelAccelStruct);

    if (!bindingSet)
    {
        bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, constantBuffer))
            .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, scene.gpu.topLevelAccelStruct))
            .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(1, scene.gpu.materialBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(2, scene.gpu.meshBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(3, scene.gpu.normalBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(4, scene.gpu.tangentBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(5, scene.gpu.texCoordBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(6, scene.gpu.colorBuffer))
            .addItem(nvrhi::BindingSetItem::TypedBuffer_SRV(7, scene.gpu.indexBuffer))
            .addItem(nvrhi::BindingSetItem::Texture_UAV(0, texture))
            .addItem(nvrhi::BindingSetItem::Sampler(0, sampler)), bindingLayout);

        descriptorTable = app.device.nvrhi->createDescriptorTable(bindlessLayout);

        app.device.nvrhi->resizeDescriptorTable(descriptorTable, (uint32_t)scene.gpu.textures.size(), false);
        for (size_t i = 0; i < scene.gpu.textures.size(); i++)
            app.device.nvrhi->writeDescriptorTable(descriptorTable, nvrhi::BindingSetItem::Texture_SRV((uint32_t)i, scene.gpu.textures[i]));
    }

    commandList->open();

    ConstantBuffer bufferData;
    bufferData.position = app.camera.position;
    bufferData.tanFovy = tan(app.camera.fieldOfView / 360.0f * 3.14159265359f);
    bufferData.rotation.block(0, 0, 3, 3) = app.camera.rotation.toRotationMatrix();
    bufferData.aspectRatio = (float)app.window.width / (float)app.window.height;

    commandList->writeBuffer(constantBuffer, &bufferData, sizeof(ConstantBuffer));

    commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
		.addBindingSet(descriptorTable));

    commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(app.window.width)
        .setHeight(app.window.height));

    commandList->copyTexture(
        app.window.getCurrentSwapChainTexture(),
        nvrhi::TextureSlice(),
        texture,
        nvrhi::TextureSlice());

    commandList->close();

    app.device.nvrhi->executeCommandList(commandList);
}
