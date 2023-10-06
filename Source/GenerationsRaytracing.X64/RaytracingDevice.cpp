#include "RaytracingDevice.h"

#include "DXILLibrary.h"
#include "CopyTextureVertexShader.h"
#include "CopyTexturePixelShader.h"
#include "EnvironmentColor.h"
#include "GeometryFlags.h"
#include "Message.h"
#include "RootSignature.h"
#include "PoseComputeShader.h"

static constexpr uint32_t SCRATCH_BUFFER_SIZE = 16 * 1024 * 1024;

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

uint32_t RaytracingDevice::buildRaytracingAccelerationStructure(ComPtr<D3D12MA::Allocation>& allocation,
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&accelStructDesc.Inputs, &preBuildInfo);

    if (allocation == nullptr || allocation->GetSize() < preBuildInfo.ResultDataMaxSizeInBytes)
    {
        if (allocation != nullptr)
            m_tempBuffers[m_frame].push_back(std::move(allocation));

        createBuffer(
            D3D12_HEAP_TYPE_DEFAULT,
            static_cast<uint32_t>(preBuildInfo.ResultDataMaxSizeInBytes),
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            allocation);
    }

    accelStructDesc.DestAccelerationStructureData = allocation->GetResource()->GetGPUVirtualAddress();

    if (buildImmediate)
    {
        assert(m_uploadBufferOffset + preBuildInfo.ScratchDataSizeInBytes <= SCRATCH_BUFFER_SIZE);

        accelStructDesc.ScratchAccelerationStructureData = m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

        m_scratchBufferOffset = static_cast<uint32_t>((m_scratchBufferOffset + preBuildInfo.ScratchDataSizeInBytes
            + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1));

        getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
        getGraphicsCommandList().uavBarrier(allocation->GetResource());
    }

    return static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes);
}

void RaytracingDevice::handlePendingBottomLevelAccelStructBuilds()
{
    for (const auto pendingPose : m_pendingPoses)
        getGraphicsCommandList().uavBarrier(m_vertexBuffers[pendingPose].allocation->GetResource());

    getGraphicsCommandList().commitBarriers();

    for (const auto pendingBuild : m_pendingBuilds)
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[pendingBuild];

        bottomLevelAccelStruct.desc.ScratchAccelerationStructureData =
            m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

        m_scratchBufferOffset = m_scratchBufferOffset + bottomLevelAccelStruct.scratchBufferSize
            + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1 & ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1);

        getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelAccelStruct.desc, 0, nullptr);
        getGraphicsCommandList().uavBarrier(bottomLevelAccelStruct.allocation->GetResource());
    }

    m_pendingPoses.clear();
    m_pendingBuilds.clear();
}

static float haltonSequence(int i, int b)
{
    float f = 1.0;
    float r = 0.0;

    while (i > 0)
    {
        f = f / float(b);
        r = r + f * float(i % b);
        i = i / b;
    }

    return r;
}

static void haltonJitter(int frame, int phases, float& jitterX, float& jitterY)
{
    jitterX = haltonSequence(frame % phases + 1, 2) - 0.5f;
    jitterY = haltonSequence(frame % phases + 1, 3) - 0.5f;
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createGlobalsRT()
{
    EnvironmentColor::get(m_globalsPS, m_globalsRT.environmentColor);

    haltonJitter(m_globalsRT.currentFrame, 64, m_globalsRT.pixelJitterX, m_globalsRT.pixelJitterY);

    // TODO: Preferably do this check in Generations
    static float prevMatrices[32];
    bool allMatch = true;

    for (size_t i = 0; i < 32; i++)
    {
        allMatch &= abs(prevMatrices[i] - (&m_globalsVS.floatConstants[0][0])[i]) < 0.001f;
        prevMatrices[i] = (&m_globalsVS.floatConstants[0][0])[i];
    }
    if (allMatch)
        ++m_globalsRT.currentFrame;
    else
        m_globalsRT.currentFrame = 0;

    memcpy(prevMatrices, &m_globalsVS, sizeof(prevMatrices));

    return createBuffer(&m_globalsRT, sizeof(GlobalsRT), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

void RaytracingDevice::createTopLevelAccelStruct()
{
    handlePendingBottomLevelAccelStructBuilds();

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

    if (m_instanceDescs.empty())
        return;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
    accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    accelStructDesc.Inputs.NumDescs = static_cast<UINT>(m_instanceDescs.size());
    accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    accelStructDesc.Inputs.InstanceDescs = createBuffer(m_instanceDescs.data(),
        static_cast<uint32_t>(m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

    getGraphicsCommandList().commitBarriers();
    buildRaytracingAccelerationStructure(m_topLevelAccelStruct, accelStructDesc, true);
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
    getUnderlyingGraphicsCommandList()->SetPipelineState(m_copyTexturePipeline.Get());
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

    if (m_bottomLevelAccelStructs.size() <= message.bottomLevelAccelStructId)
        m_bottomLevelAccelStructs.resize(message.bottomLevelAccelStructId + 1);

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];

    bottomLevelAccelStruct.geometryDescs.resize(message.dataSize / sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));
    bottomLevelAccelStruct.geometryCount = static_cast<uint32_t>(bottomLevelAccelStruct.geometryDescs.size());
    bottomLevelAccelStruct.geometryId = allocateGeometryDescs(bottomLevelAccelStruct.geometryCount);

    for (size_t i = 0; i < bottomLevelAccelStruct.geometryDescs.size(); i++)
    {
        const auto& geometryDesc =
            reinterpret_cast<const MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data)[i];

        assert((geometryDesc.indexCount % 3) == 0);
        assert(m_indexBuffers[geometryDesc.indexBufferId].byteSize >= geometryDesc.indexCount * sizeof(uint16_t));
        assert(m_vertexBuffers[geometryDesc.vertexBufferId].byteSize - geometryDesc.positionOffset >= geometryDesc.vertexCount * geometryDesc.vertexStride);
        assert((geometryDesc.positionOffset % 4) == 0);

        // BVH GeometryDesc
        {
            auto& dstGeometryDesc = bottomLevelAccelStruct.geometryDescs[i];

            dstGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

            dstGeometryDesc.Flags = (geometryDesc.flags & GEOMETRY_FLAG_TRANSPARENT) ? D3D12_RAYTRACING_GEOMETRY_FLAG_NONE :
                D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

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

            dstGeometryDesc.flags = geometryDesc.flags;
            dstGeometryDesc.indexBufferId = m_indexBuffers[geometryDesc.indexBufferId].srvIndex;
            dstGeometryDesc.vertexBufferId = m_vertexBuffers[geometryDesc.vertexBufferId].srvIndex;
            dstGeometryDesc.vertexStride = geometryDesc.vertexStride;
            dstGeometryDesc.normalOffset = geometryDesc.normalOffset;
            dstGeometryDesc.tangentOffset = geometryDesc.tangentOffset;
            dstGeometryDesc.binormalOffset = geometryDesc.binormalOffset;
            memcpy(dstGeometryDesc.texCoordOffsets, geometryDesc.texCoordOffsets, sizeof(dstGeometryDesc.texCoordOffsets));
            dstGeometryDesc.colorOffset = geometryDesc.colorOffset;
            dstGeometryDesc.materialId = geometryDesc.materialId;
        }
    }

    bottomLevelAccelStruct.desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelAccelStruct.desc.Inputs.Flags = message.preferFastBuild ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    bottomLevelAccelStruct.desc.Inputs.NumDescs = static_cast<UINT>(bottomLevelAccelStruct.geometryDescs.size());
    bottomLevelAccelStruct.desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelAccelStruct.desc.Inputs.pGeometryDescs = bottomLevelAccelStruct.geometryDescs.data();
    bottomLevelAccelStruct.scratchBufferSize = buildRaytracingAccelerationStructure(bottomLevelAccelStruct.allocation, bottomLevelAccelStruct.desc, false);
    m_pendingBuilds.push_back(message.bottomLevelAccelStructId);
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct:
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.resourceId];
        m_tempBuffers[m_frame].emplace_back(std::move(bottomLevelAccelStruct.allocation));
        freeGeometryDescs(m_bottomLevelAccelStructs[message.resourceId].geometryId, m_bottomLevelAccelStructs[message.resourceId].geometryCount);
        bottomLevelAccelStruct.desc = {};
        bottomLevelAccelStruct.geometryDescs.clear();
        break;
    }

    case MsgReleaseRaytracingResource::ResourceType::Instance:
        memset(&m_instanceDescs[message.resourceId], 0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
        break;

    case MsgReleaseRaytracingResource::ResourceType::Material:
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

    m_globalsRT.blueNoiseTextureId = m_textures[message.blueNoiseTextureId].srvIndex;

    if (m_width != message.width || m_height != message.height)
    {
        m_width = message.width;
        m_height = message.height;
        createRenderTargetAndDepthStencil();
    }

    createTopLevelAccelStruct();

    const auto globalsVS = createBuffer(&m_globalsVS,
        sizeof(m_globalsVS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsPS = createBuffer(&m_globalsPS, 
        offsetof(GlobalsPS, textureIndices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsRT = createGlobalsRT();

    const auto topLevelAccelStruct = m_topLevelAccelStruct != nullptr ? 
        m_topLevelAccelStruct->GetResource()->GetGPUVirtualAddress() : NULL;

    const auto geometryDescs = createBuffer(m_geometryDescs.data(), 
        static_cast<uint32_t>(m_geometryDescs.size() * sizeof(GeometryDesc)), 0x4);

    const auto materials = createBuffer(m_materials.data(),
        static_cast<uint32_t>(m_materials.size() * sizeof(Material)), 0x4);

    if (m_curRootSignature != m_raytracingRootSignature.Get())
    {
        getUnderlyingGraphicsCommandList()->SetComputeRootSignature(m_raytracingRootSignature.Get());
        m_curRootSignature = m_raytracingRootSignature.Get();
    }
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
        { message.diffuseTexture,       material.diffuseTexture },
        { message.specularTexture,      material.specularTexture },
        { message.specularPowerTexture, material.specularPowerTexture },
        { message.normalTexture,        material.normalTexture },
        { message.emissionTexture,      material.displacementTexture },
        { message.diffuseBlendTexture,  material.diffuseBlendTexture },
        { message.powerBlendTexture,    material.powerBlendTexture },
        { message.normalBlendTexture,   material.normalBlendTexture },
        { message.environmentTexture,   material.environmentTexture },
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
        dstTexture.texCoordIndex = srcTexture.texCoordIndex;
    }

    memcpy(material.diffuseColor, message.diffuseColor, sizeof(material.diffuseColor));
    memcpy(material.specularColor, message.specularColor, sizeof(material.specularColor));
    material.specularPower = message.specularPower;
    memcpy(material.falloffParam, message.falloffParam, sizeof(material.falloffParam));
}

void RaytracingDevice::procMsgBuildBottomLevelAccelStruct()
{
    const auto& message = m_messageReceiver.getMessage<MsgBuildBottomLevelAccelStruct>();
    m_pendingBuilds.push_back(message.bottomLevelAccelStructId);
}

void RaytracingDevice::procMsgComputePose()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputePose>();

    if (m_curRootSignature != m_poseRootSignature.Get())
    {
        getUnderlyingGraphicsCommandList()->SetComputeRootSignature(m_poseRootSignature.Get());
        m_curRootSignature = m_poseRootSignature.Get();
    }
    if (m_curPipeline != m_posePipeline.Get())
    {
        getUnderlyingGraphicsCommandList()->SetPipelineState(m_posePipeline.Get());
        m_curPipeline = m_posePipeline.Get();
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    }

    getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(0,
        createBuffer(message.data, message.nodeCount * sizeof(float[4][4]), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

    auto geometryDesc = reinterpret_cast<const MsgComputePose::GeometryDesc*>(
        message.data + message.nodeCount * sizeof(float[4][4]));

    const auto destVertexBuffer = m_vertexBuffers[message.vertexBufferId].allocation->GetResource();
    auto destVirtualAddress = destVertexBuffer->GetGPUVirtualAddress();

    for (size_t i = 0; i < message.geometryCount; i++)
    {
        struct GeometryDesc
        {
            uint32_t vertexCount;
            uint32_t vertexStride;
            uint32_t normalOffset;
            uint32_t tangentOffset;
            uint32_t binormalOffset;
            uint32_t blendWeightOffset;
            uint32_t blendIndicesOffset;
            uint32_t padding0;
            uint32_t nodePalette[25];
        };

        GeometryDesc dstGeometryDesc;
        dstGeometryDesc.vertexCount = geometryDesc->vertexCount;
        dstGeometryDesc.vertexStride = geometryDesc->vertexStride;
        dstGeometryDesc.normalOffset = geometryDesc->normalOffset;
        dstGeometryDesc.tangentOffset = geometryDesc->tangentOffset;
        dstGeometryDesc.binormalOffset = geometryDesc->binormalOffset;
        dstGeometryDesc.blendWeightOffset = geometryDesc->blendWeightOffset;
        dstGeometryDesc.blendIndicesOffset = geometryDesc->blendIndicesOffset;

        for (size_t j = 0; j < 25; j++)
            dstGeometryDesc.nodePalette[j] = geometryDesc->nodePalette[j];

        getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(1,
            createBuffer(&dstGeometryDesc, sizeof(GeometryDesc), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(2,
            m_vertexBuffers[geometryDesc->vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress());

        getUnderlyingGraphicsCommandList()->SetComputeRootUnorderedAccessView(3, destVirtualAddress);

        getUnderlyingGraphicsCommandList()->Dispatch((geometryDesc->vertexCount + 63) / 64, 1, 1);

        destVirtualAddress += geometryDesc->vertexCount * geometryDesc->vertexStride;
        ++geometryDesc;
    }
    m_pendingPoses.push_back(message.vertexBufferId);
}

bool RaytracingDevice::processRaytracingMessage()
{
    switch (m_messageReceiver.getId())
    {
    case MsgCreateBottomLevelAccelStruct::s_id: procMsgCreateBottomLevelAccelStruct(); break;
    case MsgReleaseRaytracingResource::s_id: procMsgReleaseRaytracingResource(); break;
    case MsgCreateInstance::s_id: procMsgCreateInstance(); break;
    case MsgTraceRays::s_id: procMsgTraceRays(); break;
    case MsgCreateMaterial::s_id: procMsgCreateMaterial(); break;
    case MsgComputePose::s_id: procMsgComputePose(); break;
    case MsgBuildBottomLevelAccelStruct::s_id: procMsgBuildBottomLevelAccelStruct(); break;
    default: return false;
    }

    return true;
}

void RaytracingDevice::releaseRaytracingResources()
{
    m_curRootSignature = nullptr;
    m_curPipeline = nullptr;
    m_scratchBufferOffset = 0;
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

    for (auto& scratchBuffer : m_scratchBuffers)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
            SCRATCH_BUFFER_SIZE, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            scratchBuffer.GetAddressOf(),
            IID_ID3D12Resource,
            nullptr);

        assert(SUCCEEDED(hr) && scratchBuffer != nullptr);
    }

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

    hr = m_device->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(m_copyTexturePipeline.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_copyTexturePipeline != nullptr);

    CD3DX12_ROOT_PARAMETER poseRootParams[4];
    poseRootParams[0].InitAsConstantBufferView(0);
    poseRootParams[1].InitAsConstantBufferView(1);
    poseRootParams[2].InitAsShaderResourceView(0);
    poseRootParams[3].InitAsUnorderedAccessView(0);

    RootSignature::create(
        m_device.Get(),
        poseRootParams,
        _countof(poseRootParams),
        D3D12_ROOT_SIGNATURE_FLAG_NONE,
        m_poseRootSignature);

    D3D12_COMPUTE_PIPELINE_STATE_DESC posePipelineDesc{};
    posePipelineDesc.pRootSignature = m_poseRootSignature.Get();
    posePipelineDesc.CS.pShaderBytecode = PoseComputeShader;
    posePipelineDesc.CS.BytecodeLength = sizeof(PoseComputeShader);

    hr = m_device->CreateComputePipelineState(&posePipelineDesc, IID_PPV_ARGS(m_posePipeline.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_posePipeline != nullptr);
}
