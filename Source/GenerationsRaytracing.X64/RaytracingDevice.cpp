#include "RaytracingDevice.h"

#include "DXILLibrary.h"
#include "CopyTextureVertexShader.h"
#include "CopyTexturePixelShader.h"
#include "Message.h"
#include "RootSignature.h"

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createTopLevelAccelStruct()
{
    if (m_instanceDescs.empty())
        return NULL;

    for (const auto& [instanceId, bottomLevelAccelStructId] : m_deferredInstances)
    {
        if (m_bottomLevelAccelStructs.size() > bottomLevelAccelStructId &&
            m_bottomLevelAccelStructs[bottomLevelAccelStructId] != nullptr)
        {
            m_instanceDescs[instanceId].InstanceMask = 1;
            m_instanceDescs[instanceId].AccelerationStructure =
                m_bottomLevelAccelStructs[bottomLevelAccelStructId]->GetResource()->GetGPUVirtualAddress();
        }
        else
        {
            // Still not loaded, leave it for next creation
            m_tempDeferredInstances.emplace_back(instanceId, bottomLevelAccelStructId);
        }
    }

    m_deferredInstances.clear();
    std::swap(m_deferredInstances, m_tempDeferredInstances);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
    accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    accelStructDesc.Inputs.NumDescs = static_cast<UINT>(m_instanceDescs.size());
    accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    accelStructDesc.Inputs.InstanceDescs = makeBuffer(m_instanceDescs.data(),
        static_cast<uint32_t>(m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&accelStructDesc.Inputs, &preBuildInfo);

    // TODO: Duplicate code is bad!

    auto& topLevelAccelStruct = m_tempBuffers[m_frame].emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ResultDataMaxSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        topLevelAccelStruct);

    auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        scratchBuffer);

    accelStructDesc.DestAccelerationStructureData = topLevelAccelStruct->GetResource()->GetGPUVirtualAddress();
    accelStructDesc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();

    if (!m_bottomLevelAccelStructBarriers.empty())
    {
        getUnderlyingGraphicsCommandList()->ResourceBarrier(
            static_cast<uint32_t>(m_bottomLevelAccelStructBarriers.size()), m_bottomLevelAccelStructBarriers.data());

        m_bottomLevelAccelStructBarriers.clear();
    }

    getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    getGraphicsCommandList().uavBarrier(topLevelAccelStruct->GetResource());

    return topLevelAccelStruct->GetResource()->GetGPUVirtualAddress();
}

void RaytracingDevice::createRenderTargetAndDepthStencil()
{
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_UNKNOWN,
        m_width, m_height, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    struct
    {
        DXGI_FORMAT format;
        ComPtr<D3D12MA::Allocation>& allocation;
        uint32_t& uavId;
    }
    const textureDescs[] =
    {
        { DXGI_FORMAT_R32G32B32A32_FLOAT, m_renderTargetTexture, m_renderTargetTextureId },
        { DXGI_FORMAT_R32_FLOAT, m_depthStencilTexture, m_depthStencilTextureId }
    };

    for (const auto& textureDesc : textureDescs)
    {
        resourceDesc.Format = textureDesc.format;

        const HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            textureDesc.allocation.ReleaseAndGetAddressOf(),
            IID_ID3D12Resource,
            nullptr);

        assert(SUCCEEDED(hr) && textureDesc.allocation != nullptr);

        if (textureDesc.uavId == NULL)
            textureDesc.uavId = m_descriptorHeap.allocate();

        m_device->CreateUnorderedAccessView(
            textureDesc.allocation->GetResource(),
            nullptr,
            nullptr,
            m_descriptorHeap.getCpuHandle(textureDesc.uavId));
    }
}

void RaytracingDevice::copyRenderTargetAndDepthStencil()
{
    const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

    getUnderlyingGraphicsCommandList()->SetGraphicsRootSignature(m_copyTextureRootSignature.Get());
    getUnderlyingGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, m_descriptorHeap.getGpuHandle(m_renderTargetTextureId));
    getUnderlyingGraphicsCommandList()->SetGraphicsRootDescriptorTable(1, m_descriptorHeap.getGpuHandle(m_depthStencilTextureId));
    getUnderlyingGraphicsCommandList()->OMSetRenderTargets(1, &m_renderTargetView, FALSE, &m_depthStencilView);
    getUnderlyingGraphicsCommandList()->SetPipelineState(m_copyTexturePipelineState.Get());
    getUnderlyingGraphicsCommandList()->RSSetViewports(1, &viewport);
    getUnderlyingGraphicsCommandList()->RSSetScissorRects(1, &scissorRect);
    getUnderlyingGraphicsCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    getGraphicsCommandList().commitBarriers();
    getUnderlyingGraphicsCommandList()->DrawInstanced(6, 1, 0, 0);

    m_dirtyFlags |= 
        DIRTY_FLAG_ROOT_SIGNATURE |
        DIRTY_FLAG_PIPELINE_DESC | 
        DIRTY_FLAG_GLOBALS_PS | 
        DIRTY_FLAG_GLOBALS_VS | 
        DIRTY_FLAG_VIEWPORT | 
        DIRTY_FLAG_SCISSOR_RECT |
        DIRTY_FLAG_PRIMITIVE_TOPOLOGY;

    m_dirtyFlags &= ~DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;
}

void RaytracingDevice::procMsgCreateBottomLevelAccelStruct()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateBottomLevelAccelStruct>();

    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(
        message.dataSize / sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    for (size_t i = 0; i < geometryDescs.size(); i++)
    {
        const auto& srcGeometryDesc =
            reinterpret_cast<const MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data)[i];

        auto& dstGeometryDesc = geometryDescs[i];

        dstGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        dstGeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

        auto& triangles = dstGeometryDesc.Triangles;

        triangles.Transform3x4 = NULL;
        triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
        triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        triangles.IndexCount = srcGeometryDesc.indexCount;
        triangles.VertexCount = srcGeometryDesc.vertexCount;
        triangles.IndexBuffer = m_indexBuffers[srcGeometryDesc.indexBufferId].allocation->GetResource()->GetGPUVirtualAddress();
        triangles.VertexBuffer.StartAddress = m_vertexBuffers[srcGeometryDesc.vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + srcGeometryDesc.vertexOffset;
        triangles.VertexBuffer.StrideInBytes = srcGeometryDesc.vertexStride;
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
    accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    accelStructDesc.Inputs.NumDescs = static_cast<UINT>(geometryDescs.size());
    accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    accelStructDesc.Inputs.pGeometryDescs = geometryDescs.data();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&accelStructDesc.Inputs, &preBuildInfo);

    // TODO: suballocate all of this
    if (m_bottomLevelAccelStructs.size() <= message.bottomLevelAccelStructId)
        m_bottomLevelAccelStructs.resize(message.bottomLevelAccelStructId + 1);

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ResultDataMaxSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        bottomLevelAccelStruct);

    auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        scratchBuffer);

    accelStructDesc.DestAccelerationStructureData = bottomLevelAccelStruct->GetResource()->GetGPUVirtualAddress();
    accelStructDesc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();

    getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    m_bottomLevelAccelStructBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelStruct->GetResource()));
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct:
        m_tempBuffers[m_frame].emplace_back(std::move(m_bottomLevelAccelStructs[message.resourceId]));
        break;

    case MsgReleaseRaytracingResource::ResourceType::Instance:
        memset(&m_instanceDescs[message.resourceId], 0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        break;

    default:
        assert(false);
        break;
    }
}

void RaytracingDevice::procMsgCreateInstance()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateInstance>();

    if (m_instanceDescs.size() <= message.instanceId)
        m_instanceDescs.resize(message.instanceId + 1);

    auto& instanceDesc = m_instanceDescs[message.instanceId];

    memcpy(instanceDesc.Transform, message.transform, sizeof(message.transform));
    instanceDesc.InstanceID = message.instanceId;

    if (m_bottomLevelAccelStructs.size() <= message.bottomLevelAccelStructId ||
        m_bottomLevelAccelStructs[message.bottomLevelAccelStructId] == nullptr)
    {
        // Not loaded yet, defer assignment to top level acceleration structure creation
        m_deferredInstances.emplace_back(message.instanceId, message.bottomLevelAccelStructId);
    }
    else
    {
        instanceDesc.InstanceMask = 1;
        instanceDesc.AccelerationStructure = m_bottomLevelAccelStructs[
            message.bottomLevelAccelStructId]->GetResource()->GetGPUVirtualAddress();
    }
}

void RaytracingDevice::procMsgTraceRays()
{
    const auto& message = m_messageReceiver.getMessage<MsgTraceRays>();

    if (m_width != message.width || m_height != message.height)
    {
        m_width = message.width;
        m_height = message.height;
        createRenderTargetAndDepthStencil();
    }

    getUnderlyingGraphicsCommandList()->SetComputeRootSignature(m_raytracingRootSignature.Get());
    getUnderlyingGraphicsCommandList()->SetPipelineState1(m_stateObject.Get());

    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(0, 
        makeBuffer(&m_globalsVS, sizeof(m_globalsVS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(1,
        makeBuffer(&m_globalsPS, offsetof(GlobalsPS, textureIndices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(2, createTopLevelAccelStruct());

    getUnderlyingGraphicsCommandList()->SetComputeRootDescriptorTable(3, m_descriptorHeap.getGpuHandle(m_renderTargetTextureId));
    getUnderlyingGraphicsCommandList()->SetComputeRootDescriptorTable(4, m_descriptorHeap.getGpuHandle(m_depthStencilTextureId));

    const auto shaderTable = makeBuffer(
        m_shaderTable.data(), static_cast<uint32_t>(m_shaderTable.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};

    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = shaderTable;
    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.MissShaderTable.StartAddress = shaderTable + 0x40;
    dispatchRaysDesc.MissShaderTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.HitGroupTable.StartAddress = shaderTable + 0x80;
    dispatchRaysDesc.HitGroupTable.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.HitGroupTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.Width = message.width;
    dispatchRaysDesc.Height = message.height;
    dispatchRaysDesc.Depth = 1;

    getGraphicsCommandList().commitBarriers();
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);

    getGraphicsCommandList().uavBarrier(m_renderTargetTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_depthStencilTexture->GetResource());

    copyRenderTargetAndDepthStencil();
}

bool RaytracingDevice::processRaytracingMessage()
{
    switch (m_messageReceiver.getId())
    {
    case MsgCreateBottomLevelAccelStruct::s_id:
        procMsgCreateBottomLevelAccelStruct();
        break;

    case MsgReleaseRaytracingResource::s_id:
        procMsgReleaseRaytracingResource();
        break;

    case MsgCreateInstance::s_id:
        procMsgCreateInstance();
        break;

    case MsgTraceRays::s_id:
        procMsgTraceRays();
        break;

    default:
        return false;
    }

    return true;
}

RaytracingDevice::RaytracingDevice()
{
    CD3DX12_DESCRIPTOR_RANGE renderTargetDescriptorRanges[1];
    renderTargetDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE depthStencilDescriptorRanges[1];
    depthStencilDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1);

    CD3DX12_ROOT_PARAMETER raytracingRootParams[5];
    raytracingRootParams[0].InitAsConstantBufferView(0);
    raytracingRootParams[1].InitAsConstantBufferView(1);
    raytracingRootParams[2].InitAsShaderResourceView(0);
    raytracingRootParams[3].InitAsDescriptorTable(_countof(renderTargetDescriptorRanges), renderTargetDescriptorRanges);
    raytracingRootParams[4].InitAsDescriptorTable(_countof(depthStencilDescriptorRanges), depthStencilDescriptorRanges);

    RootSignature::create(
        m_device.Get(),
        raytracingRootParams,
        _countof(raytracingRootParams),
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | 
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED,
        m_raytracingRootSignature);

    CD3DX12_STATE_OBJECT_DESC stateObject(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    CD3DX12_DXIL_LIBRARY_SUBOBJECT dxilLibrarySubobject(stateObject);
    const CD3DX12_SHADER_BYTECODE dxilLibrary(DXILLibrary, sizeof(DXILLibrary));
    dxilLibrarySubobject.SetDXILLibrary(&dxilLibrary);

    CD3DX12_HIT_GROUP_SUBOBJECT hitGroupSubobject(stateObject);
    hitGroupSubobject.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    hitGroupSubobject.SetHitGroupExport(L"HitGroup");
    hitGroupSubobject.SetClosestHitShaderImport(L"ClosestHit");

    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject(stateObject);
    shaderConfigSubobject.Config(sizeof(float) * 2, sizeof(float) * 2);

    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT pipelineConfigSubobject(stateObject);
    pipelineConfigSubobject.Config(1);

    CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT rootSignatureSubobject(stateObject);
    rootSignatureSubobject.SetRootSignature(m_raytracingRootSignature.Get());

    HRESULT hr = m_device->CreateStateObject(stateObject, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_stateObject != nullptr);

    ComPtr<ID3D12StateObjectProperties> properties;
    hr = m_stateObject.As(std::addressof(properties));

    assert(SUCCEEDED(hr) && properties != nullptr);

    m_shaderTable.resize(6 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    memcpy(&m_shaderTable[0x00], properties->GetShaderIdentifier(L"RayGeneration"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&m_shaderTable[0x40], properties->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(&m_shaderTable[0x80], properties->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    CD3DX12_ROOT_PARAMETER copyRootParams[2];
    copyRootParams[0].InitAsDescriptorTable(_countof(renderTargetDescriptorRanges), renderTargetDescriptorRanges);
    copyRootParams[1].InitAsDescriptorTable(_countof(depthStencilDescriptorRanges), depthStencilDescriptorRanges);

    RootSignature::create(
        m_device.Get(),
        copyRootParams,
        _countof(copyRootParams),
        D3D12_ROOT_SIGNATURE_FLAG_NONE,
        m_copyTextureRootSignature);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.pRootSignature = m_copyTextureRootSignature.Get();
    pipelineDesc.VS.pShaderBytecode = CopyTextureVertexShader;
    pipelineDesc.VS.BytecodeLength = sizeof(CopyTextureVertexShader);
    pipelineDesc.PS.pShaderBytecode = CopyTexturePixelShader;
    pipelineDesc.PS.BytecodeLength = sizeof(CopyTexturePixelShader);
    pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineDesc.SampleMask = ~0;
    pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    pipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pipelineDesc.SampleDesc.Count = 1;

    hr = m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_copyTexturePipelineState.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_copyTexturePipelineState != nullptr);
}
