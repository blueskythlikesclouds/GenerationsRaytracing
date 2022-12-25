#include "Raytracing.h"

#include "Bridge.h"
#include "Message.h"
#include "RaytracingShaderLibrary.h"

void Raytracing::procMsgCreateGeometry(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateGeometry>();

    auto& geometryDesc = blasDescs[msg->bottomLevelAS].bottomLevelGeometries.emplace_back();

    if (msg->opaque)
        geometryDesc.flags = nvrhi::rt::GeometryFlags::Opaque;

    auto& triangles = geometryDesc.geometryData.triangles;

    triangles.indexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->indexBuffer].Get());
    triangles.vertexBuffer = nvrhi::checked_cast<nvrhi::IBuffer*>(bridge.resources[msg->vertexBuffer].Get());
    assert(triangles.indexBuffer && triangles.vertexBuffer);
    triangles.indexFormat = nvrhi::Format::R16_UINT;
    triangles.vertexFormat = nvrhi::Format::RGB32_FLOAT;
    triangles.indexOffset = msg->indexOffset;
    triangles.vertexOffset = msg->vertexOffset;
    triangles.indexCount = msg->indexCount;
    triangles.vertexCount = msg->vertexCount;
    triangles.vertexStride = msg->vertexStride;
}

void Raytracing::procMsgCreateBottomLevelAS(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateBottomLevelAS>();

    auto& desc = blasDescs[msg->bottomLevelAS];
    auto& blas = bottomLevelAccelStructs[msg->bottomLevelAS];
    blas = bridge.device.nvrhi->createAccelStruct(desc);
    nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, blas, desc);
    blasDescs.erase(msg->bottomLevelAS);
}

void Raytracing::procMsgCreateInstance(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateInstance>();

    auto& instanceDesc = instanceDescs.emplace_back();

    memcpy(instanceDesc.transform, msg->transform, sizeof(instanceDesc.transform));
    instanceDesc.instanceMask = 1;
    instanceDesc.bottomLevelAS = bottomLevelAccelStructs[msg->bottomLevelAS];

    assert(instanceDesc.bottomLevelAS);
}

void Raytracing::procMsgNotifySceneTraversed(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgNotifySceneTraversed>();

    auto& colorAttachment = bridge.framebufferDesc.colorAttachments[0].texture;
    if (!colorAttachment || !bridge.pipelineDesc.VS)
        return;

    topLevelAccelStruct = bridge.device.nvrhi->createAccelStruct(nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setTopLevelMaxInstances(instanceDescs.size()));

    bridge.commandList->buildTopLevelAccelStruct(topLevelAccelStruct, instanceDescs.data(), instanceDescs.size());
    instanceDescs.clear();

    if (!shaderLibrary)
    {
        shaderLibrary = bridge.device.nvrhi->createShaderLibrary(RAYTRACING_SHADER_LIBRARY, sizeof(RAYTRACING_SHADER_LIBRARY));

        bindingLayout = bridge.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0)));

        pipeline = bridge.device.nvrhi->createRayTracingPipeline(nvrhi::rt::PipelineDesc()
            .addBindingLayout(bindingLayout)
            .addShader({ "", shaderLibrary->getShader("RayGeneration", nvrhi::ShaderType::RayGeneration) })
            .addShader({ "", shaderLibrary->getShader("Miss", nvrhi::ShaderType::Miss) })
            .addHitGroup({ "HitGroup", shaderLibrary->getShader("ClosestHit", nvrhi::ShaderType::ClosestHit) })
            .setMaxRecursionDepth(8)
            .setMaxPayloadSize(32));

        shaderTable = pipeline->createShaderTable();
        shaderTable->setRayGenerationShader("RayGeneration");
        shaderTable->addMissShader("Miss");
        shaderTable->addHitGroup("HitGroup");
    }

    if (!texture || 
        texture->getDesc().width != colorAttachment->getDesc().width ||
        texture->getDesc().height != colorAttachment->getDesc().height ||
        texture->getDesc().format != colorAttachment->getDesc().format)
    {
        auto textureDesc = colorAttachment->getDesc();
        texture = bridge.device.nvrhi->createTexture(textureDesc
            .setIsRenderTarget(false)
            .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
            .setUseClearValue(false)
            .setIsUAV(true));
    }

    bridge.processDirtyFlags();

    auto bindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, bridge.vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, topLevelAccelStruct))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(0, texture)), bindingLayout);

    bridge.commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet));

    bridge.commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(texture->getDesc().width)
        .setHeight(texture->getDesc().height));

    bridge.commandList->copyTexture(
        colorAttachment,
        nvrhi::TextureSlice(),
        texture,
        nvrhi::TextureSlice());

    bridge.dirtyFlags = DirtyFlags::All;
}
