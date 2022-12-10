#include "RaytracingPass.h"

#include "App.h"
#include "Scene.h"

RaytracingPass::RaytracingPass(const App& app)
{
    auto textureDesc = app.window.nvrhi.swapChainTextures.front()->getDesc();

    texture = app.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setIsUAV(true)
        .setIsRenderTarget(false)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true));

    linearRepeatSampler = app.device.nvrhi->createSampler(nvrhi::SamplerDesc()
        .setAllFilters(true)
        .setAllAddressModes(nvrhi::SamplerAddressMode::Repeat));

    bindingLayout = app.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
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

    bindlessLayout = app.device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
        .setVisibility(nvrhi::ShaderType::All)
        .setMaxCapacity(2048)
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addRegisterSpace(nvrhi::BindingLayoutItem::Texture_SRV(2)));

    auto pipelineDesc = nvrhi::rt::PipelineDesc()
        .addBindingLayout(bindingLayout)
        .addBindingLayout(bindlessLayout)
        .addShader({ "", app.shaderLibrary.handle->getShader("RayGeneration", nvrhi::ShaderType::RayGeneration), nullptr })
        .addShader({ "", app.shaderLibrary.handle->getShader("Miss", nvrhi::ShaderType::Miss), nullptr })
        .setMaxRecursionDepth(8)
        .setMaxPayloadSize(32);

    for (auto& shaderPair : app.shaderLibrary.shaderMapping.map)
    {
        pipelineDesc.addHitGroup(
        {
            shaderPair.second.name,
            app.shaderLibrary.handle->getShader(shaderPair.second.closestHit.c_str(), nvrhi::ShaderType::ClosestHit),
            app.shaderLibrary.handle->getShader(shaderPair.second.anyHit.c_str(), nvrhi::ShaderType::AnyHit)
        });
    }

    pipeline = app.device.nvrhi->createRayTracingPipeline(pipelineDesc);
}

void RaytracingPass::execute(const App& app, Scene& scene)
{
    if (!scene.gpu.bvh)
        scene.createGpuResources(app);

    assert(scene.gpu.bvh);

    if (!bindingSet)
    {
        bindingSet = app.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
            .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, app.renderer.constantBuffer))
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

    app.renderer.commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
        .addBindingSet(descriptorTable));

    app.renderer.commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(texture->getDesc().width)
        .setHeight(texture->getDesc().height));
}
