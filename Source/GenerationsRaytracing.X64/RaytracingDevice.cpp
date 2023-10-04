#include "RaytracingDevice.h"

#include "DXILLibrary.h"
#include "CopyTextureVertexShader.h"
#include "CopyTexturePixelShader.h"
#include "EnvironmentColor.h"
#include "Message.h"
#include "RootSignature.h"

uint32_t RaytracingDevice::allocateGeometryDescs(uint32_t count)
{
    const auto it = m_freeCounts.lower_bound(count);
    if (it != m_freeCounts.end())
    {
        const uint32_t remnant = it->first - count;
        const uint32_t index = it->second + remnant;

        const auto it2 = m_freeIndices.find(it->second);
        assert(it2 != m_freeIndices.end());

        if (remnant > 0)
        {
            auto node = m_freeCounts.extract(it);
            node.key() = remnant;
            it2->second = m_freeCounts.insert(std::move(node));
        }
        else
        {
            m_freeIndices.erase(it2);
            m_freeCounts.erase(it);
        }

        return index;
    }
    else
    {
        const uint32_t index = static_cast<uint32_t>(m_geometryDescs.size());
        m_geometryDescs.resize(m_geometryDescs.size() + count);
        return index;
    }
}

void RaytracingDevice::freeGeometryDescs(uint32_t id, uint32_t count)
{
    const auto next = m_freeIndices.lower_bound(id);
    if (next != m_freeIndices.end())
    {
        if (next != m_freeIndices.begin())
        {
            const auto prev = std::prev(next);

            // Merge previous range (prevId + prevCount) == id
            if ((prev->first + prev->second->first) == id)
            {
                id = prev->first;
                count += prev->second->first;

                m_freeCounts.erase(prev->second);
                m_freeIndices.erase(prev->first);
            }
        }
        // Merge next range (id + count) == nextId
        if ((id + count) == next->first)
        {
            count += next->second->first;
            m_freeCounts.erase(next->second);
            m_freeIndices.erase(next->first);
        }
    }
    m_freeIndices.emplace(id, m_freeCounts.emplace(count, id));
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createGlobalsRT()
{
    EnvironmentColor::get(m_globalsPS, m_globalsRT.environmentColor);

    // TODO: Preferably do this check in Generations
    static float prevMatrices[32];
    if (memcmp(prevMatrices, &m_globalsVS, sizeof(prevMatrices)) == 0)
        ++m_globalsRT.currentFrame;
    else
        m_globalsRT.currentFrame = 1;

    memcpy(prevMatrices, &m_globalsVS, sizeof(prevMatrices));

    return createBuffer(&m_globalsRT, sizeof(GlobalsRT), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createTopLevelAccelStruct()
{
    if (m_instanceDescs.empty())
        return NULL;

    for (const auto& [textureId, textureIdOffset] : m_delayedTextures)
    {
        if (m_textures.size() > textureId &&
            m_textures[textureId].allocation != nullptr)
        {
            *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_materials.data()) + textureIdOffset) = m_textures[textureId].srvIndex;
        }
        else
        {
            // Still not loaded, leave it for next creation
            m_tempDelayedTextures.emplace_back(textureId, textureIdOffset);
        }
    }

    for (const auto& [instanceId, bottomLevelAccelStructId] : m_delayedInstances)
    {
        if (m_bottomLevelAccelStructs.size() > bottomLevelAccelStructId &&
            m_bottomLevelAccelStructs[bottomLevelAccelStructId].allocation != nullptr)
        {
            auto& instanceDesc = m_instanceDescs[instanceId];
            auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[bottomLevelAccelStructId];
            instanceDesc.InstanceID = bottomLevelAccelStruct.geometryId;
            instanceDesc.InstanceMask = 1;
            instanceDesc.AccelerationStructure = bottomLevelAccelStruct.allocation->GetResource()->GetGPUVirtualAddress();
        }
        else
        {
            // Still not loaded, leave it for next creation
            m_tempDelayedInstances.emplace_back(instanceId, bottomLevelAccelStructId);
        }
    }

    m_delayedTextures.clear();
    m_delayedInstances.clear();

    std::swap(m_delayedTextures, m_tempDelayedTextures);
    std::swap(m_delayedInstances, m_tempDelayedInstances);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
    accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    accelStructDesc.Inputs.NumDescs = static_cast<UINT>(m_instanceDescs.size());
    accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    accelStructDesc.Inputs.InstanceDescs = createBuffer(m_instanceDescs.data(),
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
        D3D12_RESOURCE_STATE_COMMON,
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
    }
    const textureDescs[] =
    {
        { DXGI_FORMAT_R32G32B32A32_FLOAT, m_renderTargetTexture },
        { DXGI_FORMAT_R32_FLOAT, m_depthStencilTexture }
    };

    if (m_uavId == NULL)
        m_uavId = m_descriptorHeap.allocateMany(_countof(textureDescs));

    auto cpuHandle = m_descriptorHeap.getCpuHandle(m_uavId);

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

        m_device->CreateUnorderedAccessView(
            textureDesc.allocation->GetResource(),
            nullptr,
            nullptr,
            cpuHandle);

        cpuHandle.ptr += m_descriptorHeap.getIncrementSize();
    }
}

void RaytracingDevice::copyRenderTargetAndDepthStencil()
{
    const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

    getUnderlyingGraphicsCommandList()->SetGraphicsRootSignature(m_copyTextureRootSignature.Get());
    getUnderlyingGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, m_descriptorHeap.getGpuHandle(m_uavId));
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

    if (m_bottomLevelAccelStructs.size() <= message.bottomLevelAccelStructId)
        m_bottomLevelAccelStructs.resize(message.bottomLevelAccelStructId + 1);

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];

    bottomLevelAccelStruct.geometryCount = static_cast<uint32_t>(geometryDescs.size());
    bottomLevelAccelStruct.geometryId = allocateGeometryDescs(bottomLevelAccelStruct.geometryCount);

    for (size_t i = 0; i < geometryDescs.size(); i++)
    {
        const auto& geometryDesc =
            reinterpret_cast<const MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data)[i];

        assert((geometryDesc.indexCount % 3) == 0 && 
            m_indexBuffers[geometryDesc.indexBufferId].byteSize >= geometryDesc.indexCount * sizeof(uint16_t) &&
            m_vertexBuffers[geometryDesc.vertexBufferId].byteSize - geometryDesc.positionOffset >= geometryDesc.vertexCount * geometryDesc.vertexStride &&
            (geometryDesc.positionOffset % 4) == 0);

        // BVH GeometryDesc
        {
            auto& dstGeometryDesc = geometryDescs[i];

            dstGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            dstGeometryDesc.Flags = geometryDesc.isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

            auto& triangles = dstGeometryDesc.Triangles;

            triangles.Transform3x4 = NULL;
            triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
            triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            triangles.IndexCount = geometryDesc.indexCount;
            triangles.VertexCount = geometryDesc.vertexCount;
            triangles.IndexBuffer = m_indexBuffers[geometryDesc.indexBufferId].allocation->GetResource()->GetGPUVirtualAddress();
            triangles.VertexBuffer.StartAddress = m_vertexBuffers[geometryDesc.vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc.positionOffset;
            triangles.VertexBuffer.StrideInBytes = geometryDesc.vertexStride;
        }

        // GPU GeometryDesc
        {
            auto& dstGeometryDesc = m_geometryDescs[bottomLevelAccelStruct.geometryId + i];

            dstGeometryDesc.indexBufferId = m_indexBuffers[geometryDesc.indexBufferId].srvIndex;
            dstGeometryDesc.vertexBufferId = m_vertexBuffers[geometryDesc.vertexBufferId].srvIndex;
            dstGeometryDesc.vertexStride = geometryDesc.vertexStride;
            dstGeometryDesc.normalOffset = geometryDesc.normalOffset;
            dstGeometryDesc.tangentOffset = geometryDesc.tangentOffset;
            dstGeometryDesc.binormalOffset = geometryDesc.binormalOffset;
            dstGeometryDesc.texCoordOffset = geometryDesc.texCoordOffsets[0];
            dstGeometryDesc.colorOffset = geometryDesc.colorOffset;
            dstGeometryDesc.materialId = geometryDesc.materialId;
        }
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

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ResultDataMaxSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        bottomLevelAccelStruct.allocation);

    auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COMMON,
        scratchBuffer);

    accelStructDesc.DestAccelerationStructureData = bottomLevelAccelStruct.allocation->GetResource()->GetGPUVirtualAddress();
    accelStructDesc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();

    getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    m_bottomLevelAccelStructBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelStruct.allocation->GetResource()));
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct:
        m_tempBuffers[m_frame].emplace_back(std::move(m_bottomLevelAccelStructs[message.resourceId].allocation));
        freeGeometryDescs(m_bottomLevelAccelStructs[message.resourceId].geometryId, m_bottomLevelAccelStructs[message.resourceId].geometryCount);
        break;

    case MsgReleaseRaytracingResource::ResourceType::Instance:
        if (message.resourceId < m_instanceDescs.size())
            memset(&m_instanceDescs[message.resourceId], 0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        break;

    case MsgReleaseRaytracingResource::ResourceType::Material:
        if (message.resourceId < m_materials.size())
            memset(&m_materials[message.resourceId], 0, sizeof(Material));
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

    if (m_bottomLevelAccelStructs.size() <= message.bottomLevelAccelStructId ||
        m_bottomLevelAccelStructs[message.bottomLevelAccelStructId].allocation == nullptr)
    {
        // Not loaded yet, defer assignment to top level acceleration structure creation
        m_delayedInstances.emplace_back(message.instanceId, message.bottomLevelAccelStructId);
    }
    else
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];
        instanceDesc.InstanceID = bottomLevelAccelStruct.geometryId;
        instanceDesc.InstanceMask = 1;
        instanceDesc.AccelerationStructure = bottomLevelAccelStruct.allocation->GetResource()->GetGPUVirtualAddress();
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

    const auto globalsVS = createBuffer(&m_globalsVS,
        sizeof(m_globalsVS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsPS = createBuffer(&m_globalsPS, 
        offsetof(GlobalsPS, textureIndices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsRT = createGlobalsRT();

    const auto topLevelAccelStruct = createTopLevelAccelStruct();

    const auto geometryDescs = createBuffer(m_geometryDescs.data(), 
        static_cast<uint32_t>(m_geometryDescs.size() * sizeof(GeometryDesc)), 0x4);

    const auto materials = createBuffer(m_materials.data(),
        static_cast<uint32_t>(m_materials.size() * sizeof(Material)), 0x4);

    getUnderlyingGraphicsCommandList()->SetComputeRootSignature(m_raytracingRootSignature.Get());
    getUnderlyingGraphicsCommandList()->SetPipelineState1(m_stateObject.Get());

    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(0, globalsVS);
    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(1, globalsPS);
    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(2, globalsRT);
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(3, topLevelAccelStruct);
    getUnderlyingGraphicsCommandList()->SetComputeRootDescriptorTable(4, m_descriptorHeap.getGpuHandle(m_uavId));
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(5, geometryDescs);
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(6, materials);

    const auto shaderTable = createBuffer(
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

void RaytracingDevice::procMsgCreateMaterial()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateMaterial>();

    if (m_materials.size() <= message.materialId)
        m_materials.resize(message.materialId + 1);

    auto& material = m_materials[message.materialId];

    const std::pair<const MsgCreateMaterial::Texture&, Material::Texture&> textures[] =
    {
        { message.diffuseTexture,      material.diffuseTexture },
        { message.specularTexture,     material.specularTexture },
        { message.powerTexture,        material.powerTexture },
        { message.normalTexture,       material.normalTexture },
        { message.emissionTexture,     material.emissionTexture },
        { message.diffuseBlendTexture, material.diffuseBlendTexture },
        { message.powerBlendTexture,   material.powerBlendTexture },
        { message.normalBlendTexture,  material.normalBlendTexture },
        { message.environmentTexture,  material.environmentTexture },
    };

    D3D12_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    for (const auto& [srcTexture, dstTexture] : textures)
    {
        if (srcTexture.id != NULL)
        {
            if (m_textures.size() <= srcTexture.id || m_textures[srcTexture.id].allocation == nullptr)
            {
                // Delay texture assignment if the texture is not loaded yet
                m_delayedTextures.emplace_back(srcTexture.id,
                    reinterpret_cast<uintptr_t>(&dstTexture.id) - reinterpret_cast<uintptr_t>(m_materials.data()));
            }
            else
            {
                dstTexture.id = m_textures[srcTexture.id].srvIndex;
            }
        }

        samplerDesc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeU);
        samplerDesc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeV);

        auto& sampler = m_samplers[XXH3_64bits(&samplerDesc, sizeof(samplerDesc))];
        if (!sampler)
        {
            sampler = m_samplerDescriptorHeap.allocate();

            m_device->CreateSampler(&samplerDesc, 
                m_samplerDescriptorHeap.getCpuHandle(sampler));
        }

        dstTexture.samplerId = sampler;
    }

    memcpy(material.diffuseColor, message.diffuseColor, sizeof(material.diffuseColor));
    memcpy(material.specularColor, message.specularColor, sizeof(material.specularColor));
    material.specularPower = message.specularPower;
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

    case MsgCreateMaterial::s_id:
        procMsgCreateMaterial();
        break;

    default:
        return false;
    }

    return true;
}

RaytracingDevice::RaytracingDevice()
{
    CD3DX12_DESCRIPTOR_RANGE descriptorRanges[1];
    descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);

    CD3DX12_ROOT_PARAMETER raytracingRootParams[7];
    raytracingRootParams[0].InitAsConstantBufferView(0, 0);
    raytracingRootParams[1].InitAsConstantBufferView(1, 0);
    raytracingRootParams[2].InitAsConstantBufferView(2, 0);
    raytracingRootParams[3].InitAsShaderResourceView(0, 0);
    raytracingRootParams[4].InitAsDescriptorTable(_countof(descriptorRanges), descriptorRanges);
    raytracingRootParams[5].InitAsShaderResourceView(1, 0);
    raytracingRootParams[6].InitAsShaderResourceView(2, 0);

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
    hitGroupSubobject.SetAnyHitShaderImport(L"AnyHit");
    hitGroupSubobject.SetClosestHitShaderImport(L"ClosestHit");

    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject(stateObject);
    shaderConfigSubobject.Config(sizeof(float) * 5, sizeof(float) * 2);

    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT pipelineConfigSubobject(stateObject);
    pipelineConfigSubobject.Config(4);

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

    CD3DX12_ROOT_PARAMETER copyRootParams[1];
    copyRootParams[0].InitAsDescriptorTable(_countof(descriptorRanges), descriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

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
