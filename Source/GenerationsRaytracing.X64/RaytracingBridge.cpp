#include "RaytracingBridge.h"

#include "Bridge.h"
#include "Message.h"
#include "RaytracingShaderLibrary.h"
#include "Utilities.h"

void RaytracingBridge::procMsgCreateGeometry(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateGeometry>();

    auto& bottomLevelAS = bottomLevelAccelStructs[msg->bottomLevelAS];
    auto& geometryDesc = bottomLevelAS.desc.bottomLevelGeometries.emplace_back();

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

    auto& geometry = bottomLevelAS.geometries.emplace_back();
    geometry.vertexStride = msg->vertexStride;
    geometry.normalOffset = msg->normalOffset;
    geometry.tangentOffset = msg->tangentOffset;
    geometry.binormalOffset = msg->binormalOffset;
    geometry.texCoordOffset = msg->texCoordOffset;
    geometry.colorOffset = msg->colorOffset;
}

void RaytracingBridge::procMsgCreateBottomLevelAS(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateBottomLevelAS>();

    auto& blas = bottomLevelAccelStructs[msg->bottomLevelAS];
    blas.handle = bridge.device.nvrhi->createAccelStruct(blas.desc.setTrackLiveness(true));
    nvrhi::utils::BuildBottomLevelAccelStruct(bridge.commandList, blas.handle, blas.desc);
}

void RaytracingBridge::procMsgCreateInstance(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgCreateInstance>();

    auto& instanceDesc = instanceDescs.emplace_back();

    memcpy(instanceDesc.transform, msg->transform, sizeof(instanceDesc.transform));
    instanceDesc.instanceMask = 1;
    instanceDesc.bottomLevelAS = bottomLevelAccelStructs[msg->bottomLevelAS].handle;
    assert(instanceDesc.bottomLevelAS);
}

void RaytracingBridge::procMsgNotifySceneTraversed(Bridge& bridge)
{
    const auto msg = bridge.msgReceiver.getMsgAndMoveNext<MsgNotifySceneTraversed>();

    if (instanceDescs.empty())
        return;

    auto& colorAttachment = bridge.framebufferDesc.colorAttachments[0].texture;
    if (!colorAttachment || colorAttachment->getDesc().format != nvrhi::Format::RGBA16_FLOAT)
        return;

    if (!shaderLibrary)
    {
        shaderLibrary = bridge.device.nvrhi->createShaderLibrary(RAYTRACING_SHADER_LIBRARY, sizeof(RAYTRACING_SHADER_LIBRARY));

        bindingLayout = bridge.device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
            .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1))
            .addItem(nvrhi::BindingLayoutItem::RayTracingAccelStruct(0))
            .addItem(nvrhi::BindingLayoutItem::StructuredBuffer_SRV(1))
            .addItem(nvrhi::BindingLayoutItem::Texture_UAV(0)));

        bindlessLayout = bridge.device.nvrhi->createBindlessLayout(nvrhi::BindlessLayoutDesc()
            .setVisibility(nvrhi::ShaderType::AllRayTracing)
            .setMaxCapacity(65336)
            .addRegisterSpace(nvrhi::BindingLayoutItem::TypedBuffer_SRV(1))
            .addRegisterSpace(nvrhi::BindingLayoutItem::RawBuffer_SRV(2)));

        descriptorTable = bridge.device.nvrhi->createDescriptorTable(bindlessLayout);

        pipeline = bridge.device.nvrhi->createRayTracingPipeline(nvrhi::rt::PipelineDesc()
            .addBindingLayout(bindingLayout)
            .addBindingLayout(bindlessLayout)
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

    std::vector<Geometry> geometries;
    std::unordered_map<nvrhi::rt::IAccelStruct*, uint32_t> blasMap;

    for (auto& [model, blas] : bottomLevelAccelStructs)
    {
        const size_t index = geometries.size();

        blasMap[blas.handle] = (uint32_t)index;
        geometries.insert(geometries.end(), blas.geometries.begin(), blas.geometries.end());
    }

    bridge.device.nvrhi->resizeDescriptorTable(descriptorTable, (uint32_t)(geometries.size() * 2), false);

    uint32_t descriptorIndex = 0;

    for (auto& [model, blas] : bottomLevelAccelStructs)
    {
        for (auto& geometry : blas.desc.bottomLevelGeometries)
        {
            bridge.device.nvrhi->writeDescriptorTable(descriptorTable,
                nvrhi::BindingSetItem::TypedBuffer_SRV(descriptorIndex++, geometry.geometryData.triangles.indexBuffer));

            bridge.device.nvrhi->writeDescriptorTable(descriptorTable,
                nvrhi::BindingSetItem::RawBuffer_SRV(descriptorIndex++, geometry.geometryData.triangles.vertexBuffer));
        }
    }

    for (auto& instance : instanceDescs)
        instance.instanceID = blasMap[instance.bottomLevelAS];

    auto topLevelAccelStruct = bridge.device.nvrhi->createAccelStruct(nvrhi::rt::AccelStructDesc()
        .setIsTopLevel(true)
        .setTrackLiveness(true)
        .setTopLevelMaxInstances(instanceDescs.size()));

    bridge.commandList->buildTopLevelAccelStruct(topLevelAccelStruct, instanceDescs.data(), instanceDescs.size());

    instanceDescs.clear();

    D3D12MA::ALLOCATION_DESC allocationDesc{};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vectorByteSize(geometries));

    bridge.device.allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        geometryBufferAllocation.ReleaseAndGetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(geometryBufferAllocation && geometryBufferAllocation->GetResource());

    geometryBuffer = bridge.device.nvrhi->createHandleForNativeBuffer(
        nvrhi::ObjectTypes::D3D12_Resource,
        geometryBufferAllocation->GetResource(), 
        nvrhi::BufferDesc()
            .setByteSize(vectorByteSize(geometries))
            .setStructStride((uint32_t)vectorByteStride(geometries))
            .setCpuAccess(nvrhi::CpuAccessMode::Write));

    void* copyDest = bridge.device.nvrhi->mapBuffer(geometryBuffer, nvrhi::CpuAccessMode::Write);
    memcpy(copyDest, geometries.data(), vectorByteSize(geometries));
    bridge.device.nvrhi->unmapBuffer(geometryBuffer);

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

    bridge.vsConstants.writeBuffer(bridge.commandList, bridge.vsConstantBuffer);
    bridge.psConstants.writeBuffer(bridge.commandList, bridge.psConstantBuffer);

    auto bindingSet = bridge.device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, bridge.vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, bridge.psConstantBuffer))
        .addItem(nvrhi::BindingSetItem::RayTracingAccelStruct(0, topLevelAccelStruct))
        .addItem(nvrhi::BindingSetItem::StructuredBuffer_SRV(1, geometryBuffer))
        .addItem(nvrhi::BindingSetItem::Texture_UAV(0, texture)), bindingLayout);

    bridge.commandList->setRayTracingState(nvrhi::rt::State()
        .setShaderTable(shaderTable)
        .addBindingSet(bindingSet)
        .addBindingSet(descriptorTable));

    bridge.commandList->dispatchRays(nvrhi::rt::DispatchRaysArguments()
        .setWidth(texture->getDesc().width)
        .setHeight(texture->getDesc().height));

    bridge.commandList->copyTexture(
        colorAttachment,
        nvrhi::TextureSlice(),
        texture,
        nvrhi::TextureSlice());
}
