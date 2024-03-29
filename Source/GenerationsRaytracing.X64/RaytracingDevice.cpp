#include "RaytracingDevice.h"

#include "DXILLibrary.h"
#include "CopyTextureVertexShader.h"
#include "CopyTexturePixelShader.h"
#include "DebugView.h"
#include "DLSS.h"
#include "DLSSD.h"
#include "EnvironmentColor.h"
#include "EnvironmentMode.h"
#include "FSR2.h"
#include "GeometryFlags.h"
#include "MaterialFlags.h"
#include "Message.h"
#include "RootSignature.h"
#include "PoseComputeShader.h"
#include "ResolveComputeShader.h"
#include "SkyVertexShader.h"
#include "SkyPixelShader.h"
#include "PIXEvent.h"
#include "SmoothNormalComputeShader.h"

static constexpr uint32_t SCRATCH_BUFFER_SIZE = 32 * 1024 * 1024;
static constexpr uint32_t CUBE_MAP_RESOLUTION = 1024;

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
        m_hitGroupShaderTable.resize(m_geometryDescs.size() * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * HIT_GROUP_NUM);
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

uint32_t RaytracingDevice::buildAccelStruct(ComPtr<D3D12MA::Allocation>& allocation,
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
        // Create separate resource if it does not fit to the pool
        if (m_scratchBufferOffset + preBuildInfo.ScratchDataSizeInBytes > SCRATCH_BUFFER_SIZE)
        {
            auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

            createBuffer(
                D3D12_HEAP_TYPE_DEFAULT,
                static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes),
                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_COMMON,
                scratchBuffer);

            accelStructDesc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();
        }
        else
        {
            accelStructDesc.ScratchAccelerationStructureData = m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

            m_scratchBufferOffset = static_cast<uint32_t>((m_scratchBufferOffset + preBuildInfo.ScratchDataSizeInBytes
                + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1));
        }

        getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
        getGraphicsCommandList().uavBarrier(allocation->GetResource());
    }

    return static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes);
}

void RaytracingDevice::buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct)
{
    // Create separate resource if it does not fit to the pool
    if (m_scratchBufferOffset + bottomLevelAccelStruct.scratchBufferSize > SCRATCH_BUFFER_SIZE)
    {
        auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

        createBuffer(
            D3D12_HEAP_TYPE_DEFAULT,
            bottomLevelAccelStruct.scratchBufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON,
            scratchBuffer);

        bottomLevelAccelStruct.desc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();
    }
    else
    {
        bottomLevelAccelStruct.desc.ScratchAccelerationStructureData =
            m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

        m_scratchBufferOffset = (m_scratchBufferOffset + bottomLevelAccelStruct.scratchBufferSize
            + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1) & ~(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1);
    }

    getUnderlyingGraphicsCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelAccelStruct.desc, 0, nullptr);
    getGraphicsCommandList().uavBarrier(bottomLevelAccelStruct.allocation->GetResource());
}

void RaytracingDevice::handlePendingBottomLevelAccelStructBuilds()
{
    PIX_EVENT();

    for (const auto pendingPose : m_pendingPoses)
        getGraphicsCommandList().uavBarrier(m_vertexBuffers[pendingPose].allocation->GetResource());

    getGraphicsCommandList().commitBarriers();

    for (const auto pendingBuild : m_pendingBuilds)
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[pendingBuild];

        if (bottomLevelAccelStruct.allocation != nullptr)
            buildBottomLevelAccelStruct(bottomLevelAccelStruct);
    }

    m_pendingPoses.clear();
    m_pendingBuilds.clear();
}

void RaytracingDevice::handlePendingSmoothNormalCommands()
{
    if (m_smoothNormalCommands.empty())
        return;

    PIX_EVENT();

    if (m_curRootSignature != m_smoothNormalRootSignature.Get())
    {
        getUnderlyingGraphicsCommandList()->SetComputeRootSignature(m_smoothNormalRootSignature.Get());
        m_curRootSignature = m_smoothNormalRootSignature.Get();
    }
    if (m_curPipeline != m_smoothNormalPipeline.Get())
    {
        getUnderlyingGraphicsCommandList()->SetPipelineState(m_smoothNormalPipeline.Get());
        m_curPipeline = m_smoothNormalPipeline.Get();
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    }

    for (const auto& cmd : m_smoothNormalCommands)
    {
        struct
        {
            uint32_t indexBufferId;
            uint32_t vertexStride;
            uint32_t vertexCount;
            uint32_t normalOffset;
        } geometryDesc = 
        {
            m_indexBuffers[cmd.indexBufferId].srvIndex,
            cmd.vertexStride,
            cmd.vertexCount,
            cmd.normalOffset
        };

        getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(0,
            createBuffer(&geometryDesc, sizeof(geometryDesc), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        const auto& vertexBuffer = m_vertexBuffers[cmd.vertexBufferId];

        getUnderlyingGraphicsCommandList()->SetComputeRootUnorderedAccessView(1,
            vertexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + cmd.vertexOffset);

        getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(2,
            m_indexBuffers[cmd.adjacencyBufferId].allocation->GetResource()->GetGPUVirtualAddress());

        getUnderlyingGraphicsCommandList()->Dispatch((cmd.vertexCount + 63) / 64, 1, 1);

        getGraphicsCommandList().uavBarrier(vertexBuffer.allocation->GetResource());
    }

    m_smoothNormalCommands.clear();
}

void RaytracingDevice::writeHitGroupShaderTable(size_t geometryIndex, size_t shaderType, bool constTexCoord)
{
    uint8_t* hitGroupTable = &m_hitGroupShaderTable[geometryIndex * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * HIT_GROUP_NUM];
    void* const* hitGroups = &m_hitGroups[shaderType * (_countof(m_hitGroups) / SHADER_TYPE_MAX)];

    for (size_t i = 0; i < HIT_GROUP_NUM; i++)
    {
        const size_t index = i == 0 ? (constTexCoord ? 1 : 0) : i + 1;
        memcpy(&hitGroupTable[i * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], hitGroups[index], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createGlobalsRT(const MsgTraceRays& message)
{
    if (message.resetAccumulation || message.debugView != DEBUG_VIEW_NONE)
        m_globalsRT.currentFrame = 0;

    switch (message.envMode)
    {
    case ENVIRONMENT_MODE_AUTO:
        m_globalsRT.useEnvironmentColor = EnvironmentColor::get(m_globalsPS,
            m_globalsRT.skyColor, m_globalsRT.groundColor);

        if (m_globalsRT.useEnvironmentColor || (message.skyColor[0] <= 0.0f && 
            message.skyColor[1] <= 0.0f && message.skyColor[2] <= 0.0f))
        {
            break;
        }
        // fallthrough
    case ENVIRONMENT_MODE_COLOR:
        m_globalsRT.useEnvironmentColor = true;
        memcpy(m_globalsRT.skyColor, message.skyColor, sizeof(m_globalsRT.skyColor));
        memcpy(m_globalsRT.groundColor, message.groundColor, sizeof(m_globalsRT.groundColor));
        break;

    case ENVIRONMENT_MODE_SKY:
        m_globalsRT.useEnvironmentColor = false;
        break;

    }

    ffxFsr2GetJitterOffset(&m_globalsRT.pixelJitterX, &m_globalsRT.pixelJitterY, static_cast<int32_t>(m_globalsRT.currentFrame),
        ffxFsr2GetJitterPhaseCount(static_cast<int32_t>(m_upscaler->getWidth()), static_cast<int32_t>(m_width)));

    m_globalsRT.internalResolutionWidth = m_upscaler->getWidth();
    m_globalsRT.internalResolutionHeight = m_upscaler->getHeight();

    static std::default_random_engine engine;
    static std::uniform_int_distribution distribution(0, 63);

    m_globalsRT.blueNoiseOffsetX = message.debugView != DEBUG_VIEW_NONE ? 0 : distribution(engine);
    m_globalsRT.blueNoiseOffsetY = message.debugView != DEBUG_VIEW_NONE ? 0 : distribution(engine);
    m_globalsRT.blueNoiseOffsetZ = message.debugView != DEBUG_VIEW_NONE ? 0 : distribution(engine);
    m_globalsRT.blueNoiseTextureId = m_textures[message.blueNoiseTextureId].srvIndex;

    m_globalsRT.localLightCount = message.localLightCount;
    m_globalsRT.diffusePower = message.diffusePower;
    m_globalsRT.lightPower = message.lightPower;
    m_globalsRT.emissivePower = message.emissivePower;
    m_globalsRT.skyPower = message.skyPower;
    memcpy(m_globalsRT.backgroundColor, message.backgroundColor, sizeof(m_globalsRT.backgroundColor));
    m_globalsRT.useSkyTexture = message.useSkyTexture;
    m_globalsRT.adaptionLuminanceTextureId = m_textures[message.adaptionLuminanceTextureId].srvIndex;
    m_globalsRT.middleGray = message.middleGray;

    getGraphicsCommandList().transitionBarrier(m_textures[message.adaptionLuminanceTextureId].allocation->GetResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    const auto globalsRT = createBuffer(&m_globalsRT, sizeof(GlobalsRT), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    memcpy(m_globalsRT.prevProj, m_globalsVS.floatConstants, 0x80);
    ++m_globalsRT.currentFrame;

    return globalsRT;
}

bool RaytracingDevice::createTopLevelAccelStruct()
{
    handlePendingBottomLevelAccelStructBuilds();
    handlePendingSmoothNormalCommands();

    if (m_instanceDescs.empty())
        return false;

    PIX_EVENT();

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
    accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    accelStructDesc.Inputs.NumDescs = static_cast<UINT>(m_instanceDescs.size());
    accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    accelStructDesc.Inputs.InstanceDescs = createBuffer(m_instanceDescs.data(),
        static_cast<uint32_t>(m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

    getGraphicsCommandList().commitBarriers();
    buildAccelStruct(m_topLevelAccelStruct, accelStructDesc, true);

    return true;
}

static constexpr size_t s_uavTextureNum = 21;

void RaytracingDevice::createRaytracingTextures()
{
    m_upscaler->init({ *this, m_width, m_height, m_qualityMode });

    setDescriptorHeaps();
    m_curRootSignature = nullptr;

    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
    allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

    auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_UNKNOWN,
        0, 0, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    struct
    {
        DXGI_FORMAT format;
        ComPtr<D3D12MA::Allocation>& allocation;
        uint32_t width;
        uint32_t height;
    }
    const textureDescs[] =
    {
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_colorTexture },
        { DXGI_FORMAT_R32_FLOAT, m_depthTexture },
        { DXGI_FORMAT_R16G16_FLOAT, m_motionVectorsTexture },
        { DXGI_FORMAT_R32_FLOAT, m_exposureTexture, 1, 1 },

        { DXGI_FORMAT_R32G32B32A32_FLOAT, m_gBufferTexture0 },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture1 },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture2 },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture3 },
        { DXGI_FORMAT_R32G32B32A32_FLOAT, m_gBufferTexture4 },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture5 },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture6 },

        { DXGI_FORMAT_R8_UNORM, m_shadowTexture },
        { DXGI_FORMAT_R32G32B32A32_UINT, m_reservoirTexture },
        { DXGI_FORMAT_R32G32B32A32_UINT, m_prevReservoirTexture },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_giTexture },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_reflectionTexture },
        { DXGI_FORMAT_R16G16B16A16_FLOAT, m_refractionTexture },

        { DXGI_FORMAT_R11G11B10_FLOAT, m_diffuseAlbedoTexture },
        { DXGI_FORMAT_R11G11B10_FLOAT, m_specularAlbedoTexture },
        { DXGI_FORMAT_R16_FLOAT, m_linearDepthTexture },
        { DXGI_FORMAT_R16_FLOAT, m_specularHitDistanceTexture },
    };

    static_assert(_countof(textureDescs) == s_uavTextureNum);

    for (const auto& textureDesc : textureDescs)
    {
        resourceDesc.Width = textureDesc.width > 0 ? textureDesc.width : m_upscaler->getWidth();
        resourceDesc.Height = textureDesc.height > 0 ? textureDesc.height : m_upscaler->getHeight();
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
    }

    if (m_uavId == NULL)
        m_uavId = m_descriptorHeap.allocateMany(_countof(textureDescs));
    
    auto cpuHandle = m_descriptorHeap.getCpuHandle(m_uavId);
    
    for (const auto& textureDesc : textureDescs)
    {
        auto resource = textureDesc.allocation->GetResource();
    
        m_device->CreateUnorderedAccessView(
            resource,
            nullptr,
            nullptr,
            cpuHandle);
    
        cpuHandle.ptr += m_descriptorHeap.getIncrementSize();
    }

    resourceDesc.Width = m_width;
    resourceDesc.Height = m_height;
    resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

    const HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        m_outputTexture.ReleaseAndGetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && m_outputTexture != nullptr);

    m_debugViewTextures[DEBUG_VIEW_NONE] = m_outputTexture->GetResource();
    m_debugViewTextures[DEBUG_VIEW_DIFFUSE] = m_gBufferTexture1->GetResource();
    m_debugViewTextures[DEBUG_VIEW_SPECULAR] = m_gBufferTexture2->GetResource();
    m_debugViewTextures[DEBUG_VIEW_NORMAL] = m_gBufferTexture4->GetResource();
    m_debugViewTextures[DEBUG_VIEW_FALLOFF] = m_gBufferTexture5->GetResource();
    m_debugViewTextures[DEBUG_VIEW_EMISSION] = m_gBufferTexture6->GetResource();
    m_debugViewTextures[DEBUG_VIEW_SHADOW] = m_shadowTexture->GetResource();
    m_debugViewTextures[DEBUG_VIEW_GI] = m_giTexture->GetResource();
    m_debugViewTextures[DEBUG_VIEW_REFLECTION] = m_reflectionTexture->GetResource();
    m_debugViewTextures[DEBUG_VIEW_REFRACTION] = m_refractionTexture->GetResource();

    static_assert(_countof(m_debugViewTextures) == DEBUG_VIEW_MAX);

    for (size_t i = 0; i < DEBUG_VIEW_MAX; i++)
    {
        auto& srvId = m_srvIds[i];

        if (srvId == NULL)
            srvId = m_descriptorHeap.allocateMany(2);

        cpuHandle = m_descriptorHeap.getCpuHandle(srvId);
        m_device->CreateShaderResourceView(m_debugViewTextures[i], nullptr, cpuHandle);

        cpuHandle.ptr += m_descriptorHeap.getIncrementSize();
        m_device->CreateShaderResourceView(m_depthTexture->GetResource(), nullptr, cpuHandle);
    }
}

void RaytracingDevice::resolveAndDispatchUpscaler(const MsgTraceRays& message)
{
    PIX_BEGIN_EVENT("Resolve");

    getUnderlyingGraphicsCommandList()->SetPipelineState(m_resolvePipeline.Get());

    getUnderlyingGraphicsCommandList()->Dispatch(
        (m_upscaler->getWidth() + 7) / 8,
        (m_upscaler->getHeight() + 7) / 8,
        1);

    PIX_END_EVENT();

    if (message.debugView != DEBUG_VIEW_NONE)
        return;

    PIX_BEGIN_EVENT("Upscale");

    getGraphicsCommandList().uavBarrier(m_colorTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_exposureTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_diffuseAlbedoTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_specularAlbedoTexture->GetResource());

    getGraphicsCommandList().transitionBarrier(m_diffuseAlbedoTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_specularAlbedoTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_gBufferTexture4->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_colorTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_depthTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_motionVectorsTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_linearDepthTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_exposureTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_specularHitDistanceTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().commitBarriers();

    m_upscaler->dispatch(
        {
            *this,
            m_diffuseAlbedoTexture->GetResource(),
            m_specularAlbedoTexture->GetResource(),
            m_gBufferTexture4->GetResource(),
            m_colorTexture->GetResource(),
            m_outputTexture->GetResource(),
            m_depthTexture->GetResource(),
            m_linearDepthTexture->GetResource(),
            m_motionVectorsTexture->GetResource(),
            m_exposureTexture->GetResource(),
            m_specularHitDistanceTexture->GetResource(),
            m_globalsRT.pixelJitterX,
            m_globalsRT.pixelJitterY,
            message.resetAccumulation
        });

    PIX_END_EVENT();

    setDescriptorHeaps();
    m_curRootSignature = nullptr;
}

void RaytracingDevice::copyToRenderTargetAndDepthStencil(const MsgTraceRays& message)
{
    PIX_EVENT();

    const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

    getUnderlyingGraphicsCommandList()->OMSetRenderTargets(1, &m_renderTargetView, FALSE, &m_depthStencilView);
    getUnderlyingGraphicsCommandList()->SetGraphicsRootSignature(m_copyTextureRootSignature.Get());
    getUnderlyingGraphicsCommandList()->SetGraphicsRootDescriptorTable(0, m_descriptorHeap.getGpuHandle(m_srvIds[message.debugView]));
    getUnderlyingGraphicsCommandList()->OMSetRenderTargets(1, &m_renderTargetView, FALSE, &m_depthStencilView);
    getUnderlyingGraphicsCommandList()->SetPipelineState(m_copyTexturePipeline.Get());
    getUnderlyingGraphicsCommandList()->RSSetViewports(1, &viewport);
    getUnderlyingGraphicsCommandList()->RSSetScissorRects(1, &scissorRect);
    getUnderlyingGraphicsCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    getGraphicsCommandList().transitionBarrier(m_debugViewTextures[message.debugView],
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    getGraphicsCommandList().transitionBarrier(m_depthTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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
        assert(m_vertexBuffers[geometryDesc.vertexBufferId].byteSize - geometryDesc.vertexOffset >= geometryDesc.vertexCount * geometryDesc.vertexStride);
        assert((geometryDesc.vertexOffset % 4) == 0);

        // BVH GeometryDesc
        {
            auto& dstGeometryDesc = bottomLevelAccelStruct.geometryDescs[i];

            dstGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

            dstGeometryDesc.Flags = (geometryDesc.flags & (GEOMETRY_FLAG_TRANSPARENT | GEOMETRY_FLAG_PUNCH_THROUGH)) ? 
                D3D12_RAYTRACING_GEOMETRY_FLAG_NONE : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            auto& triangles = dstGeometryDesc.Triangles;

            triangles.Transform3x4 = NULL;
            triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
            triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            triangles.IndexCount = geometryDesc.indexCount;
            triangles.VertexCount = geometryDesc.vertexCount;
            triangles.IndexBuffer = m_indexBuffers[geometryDesc.indexBufferId].allocation->GetResource()->GetGPUVirtualAddress();
            triangles.VertexBuffer.StartAddress = m_vertexBuffers[geometryDesc.vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc.vertexOffset;
            triangles.VertexBuffer.StrideInBytes = geometryDesc.vertexStride;
        }

        // GPU GeometryDesc
        {
            auto& dstGeometryDesc = m_geometryDescs[bottomLevelAccelStruct.geometryId + i];

            dstGeometryDesc.flags = geometryDesc.flags;
            dstGeometryDesc.indexBufferId = m_indexBuffers[geometryDesc.indexBufferId].srvIndex;
            dstGeometryDesc.vertexBufferId = m_vertexBuffers[geometryDesc.vertexBufferId].srvIndex;
            dstGeometryDesc.vertexStride = geometryDesc.vertexStride;
            dstGeometryDesc.vertexCount = geometryDesc.vertexCount;
            dstGeometryDesc.vertexOffset = geometryDesc.vertexOffset;
            dstGeometryDesc.normalOffset = geometryDesc.normalOffset;
            dstGeometryDesc.tangentOffset = geometryDesc.tangentOffset;
            dstGeometryDesc.binormalOffset = geometryDesc.binormalOffset;
            dstGeometryDesc.colorOffset = geometryDesc.colorOffset;
            dstGeometryDesc.texCoordOffset0 = geometryDesc.texCoordOffsets[0];
            dstGeometryDesc.texCoordOffset1 = geometryDesc.texCoordOffsets[1];
            dstGeometryDesc.texCoordOffset2 = geometryDesc.texCoordOffsets[2];
            dstGeometryDesc.texCoordOffset3 = geometryDesc.texCoordOffsets[3];
            dstGeometryDesc.materialId = geometryDesc.materialId;
        }

        const auto& material = m_materials[geometryDesc.materialId];
        writeHitGroupShaderTable(bottomLevelAccelStruct.geometryId + i, material.shaderType, (material.flags & MATERIAL_FLAG_CONST_TEX_COORD) != 0);

        if (material.flags & MATERIAL_FLAG_DOUBLE_SIDED)
            bottomLevelAccelStruct.instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    }

    bottomLevelAccelStruct.desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelAccelStruct.desc.Inputs.Flags = message.preferFastBuild ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    bottomLevelAccelStruct.desc.Inputs.NumDescs = static_cast<UINT>(bottomLevelAccelStruct.geometryDescs.size());
    bottomLevelAccelStruct.desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelAccelStruct.desc.Inputs.pGeometryDescs = bottomLevelAccelStruct.geometryDescs.data();
    bottomLevelAccelStruct.scratchBufferSize = buildAccelStruct(bottomLevelAccelStruct.allocation, bottomLevelAccelStruct.desc, false);
    
    m_pendingBuilds.push_back(message.bottomLevelAccelStructId);
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case RaytracingResourceType::BottomLevelAccelStruct:
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.resourceId];

        m_tempBuffers[m_frame].emplace_back(std::move(bottomLevelAccelStruct.allocation));
        freeGeometryDescs(m_bottomLevelAccelStructs[message.resourceId].geometryId, m_bottomLevelAccelStructs[message.resourceId].geometryCount);

        bottomLevelAccelStruct = {};
        break;
    }

    case RaytracingResourceType::Instance:
        memset(&m_instanceDescs[message.resourceId], 0, sizeof(D3D12_RAYTRACING_INSTANCE_DESC));

        if (m_geometryRanges.size() > message.resourceId)
        {
            auto& geometryRange = m_geometryRanges[message.resourceId];
            if (geometryRange.first != NULL)
                freeGeometryDescs(geometryRange.first, geometryRange.second);

            geometryRange = {};
        }

        break;

    case RaytracingResourceType::Material:
    {
        if (m_materials.size() > message.resourceId)
            memset(&m_materials[message.resourceId], 0, sizeof(Material));
        break;
    }

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

    if (m_instanceDescsEx.size() <= message.instanceId)
        m_instanceDescsEx.resize(message.instanceId + 1);

    auto& instanceDesc = m_instanceDescs[message.instanceId];

    memcpy(m_instanceDescsEx[message.instanceId].prevTransform, 
        message.storePrevTransform ? instanceDesc.Transform : message.transform, sizeof(message.transform));

    memcpy(instanceDesc.Transform, message.transform, sizeof(message.transform));

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];

    if (message.dataSize > 0)
    {
        if (m_geometryRanges.size() <= message.instanceId)
            m_geometryRanges.resize(message.instanceId + 1);

        auto& [geometryId, geometryCount] = m_geometryRanges[message.instanceId];

        if (geometryId == NULL)
        {
            geometryId = allocateGeometryDescs(bottomLevelAccelStruct.geometryCount);
            geometryCount = bottomLevelAccelStruct.geometryCount;

            auto& geometryDesc = m_geometryDescs[geometryId];

            memcpy(&geometryDesc, &m_geometryDescs[bottomLevelAccelStruct.geometryId],
                bottomLevelAccelStruct.geometryCount * sizeof(GeometryDesc));

            for (size_t i = 0; i < geometryCount; i++)
            {
                const auto& material = m_materials[geometryDesc.materialId];
                writeHitGroupShaderTable(geometryId + i, material.shaderType, (material.flags & MATERIAL_FLAG_CONST_TEX_COORD) != 0);
            }
        }

        instanceDesc.InstanceID = geometryId;

        for (size_t i = 0; i < geometryCount; i++)
        {
            const auto& srcGeometry = m_geometryDescs[bottomLevelAccelStruct.geometryId + i];
            auto& dstGeometry = m_geometryDescs[geometryId + i];

            dstGeometry.materialId = srcGeometry.materialId;

            auto curId = reinterpret_cast<const uint32_t*>(message.data);
            const auto lastId = reinterpret_cast<const uint32_t*>(message.data + message.dataSize);

            while (curId < lastId)
            {
                if ((*curId) == srcGeometry.materialId)
                {
                    dstGeometry.materialId = *(curId + 1);
                    break;
                }
                else
                {
                    curId += 2;
                }
            }

            const auto& material = m_materials[dstGeometry.materialId];
            writeHitGroupShaderTable(geometryId + i, material.shaderType, (material.flags & MATERIAL_FLAG_CONST_TEX_COORD) != 0);
        }
    }
    else
    {
        instanceDesc.InstanceID = bottomLevelAccelStruct.geometryId;
    }

    instanceDesc.InstanceMask = 1;
    instanceDesc.InstanceContributionToHitGroupIndex = instanceDesc.InstanceID * HIT_GROUP_NUM;
    instanceDesc.Flags = bottomLevelAccelStruct.instanceFlags;

    if (message.isMirrored && !(instanceDesc.Flags & D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE))
        instanceDesc.Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

    instanceDesc.AccelerationStructure = bottomLevelAccelStruct.allocation->GetResource()->GetGPUVirtualAddress();
}

void RaytracingDevice::procMsgTraceRays()
{
    const auto& message = m_messageReceiver.getMessage<MsgTraceRays>();

    if (!createTopLevelAccelStruct())
        return;

    const bool resolutionMismatch = m_width != message.width || m_height != message.height;
    const bool upscalerMismatch = message.upscaler != 0 && m_upscaler->getType() != m_upscalerOverride[message.upscaler - 1];
    const bool qualityModeMismatch = message.qualityMode != 0 && m_qualityMode != static_cast<QualityMode>(message.qualityMode - 1);

    if (resolutionMismatch || upscalerMismatch || qualityModeMismatch)
    {
        m_width = message.width;
        m_height = message.height;

        m_graphicsQueue.wait(m_fenceValues[(m_frame - 1) % NUM_FRAMES]);

        if (upscalerMismatch)
        {
            std::unique_ptr<DLSS> upscaler;

            if (message.upscaler == static_cast<uint32_t>(UpscalerType::DLSS) + 1)
                upscaler = std::make_unique<DLSS>(*this);

            else if (message.upscaler == static_cast<uint32_t>(UpscalerType::DLSSD) + 1)
                upscaler = std::make_unique<DLSSD>(*this);

            m_upscaler = nullptr;

            if (upscaler != nullptr)
            {
                if (upscaler->valid())
                    m_upscaler = std::move(upscaler);
                else
                    m_upscalerOverride[message.upscaler - 1] = UpscalerType::FSR2;
            }

            if (m_upscaler == nullptr)
                m_upscaler = std::make_unique<FSR2>(*this);
        }

        if (qualityModeMismatch)
            m_qualityMode = static_cast<QualityMode>(message.qualityMode - 1);

        createRaytracingTextures();
    }

    const auto globalsVS = createBuffer(&m_globalsVS,
        sizeof(m_globalsVS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsPS = createBuffer(&m_globalsPS, 
        offsetof(GlobalsPS, textureIndices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsRT = createGlobalsRT(message);

    const auto topLevelAccelStruct = m_topLevelAccelStruct != nullptr ? 
        m_topLevelAccelStruct->GetResource()->GetGPUVirtualAddress() : NULL;

    const auto geometryDescs = createBuffer(m_geometryDescs.data(), 
        static_cast<uint32_t>(m_geometryDescs.size() * sizeof(GeometryDesc)), 0x10);

    const auto materials = createBuffer(m_materials.data(),
        static_cast<uint32_t>(m_materials.size() * sizeof(Material)), 0x10);

    const auto instanceDescs = createBuffer(m_instanceDescsEx.data(),
        static_cast<uint32_t>(m_instanceDescsEx.size() * sizeof(InstanceDesc)), 0x10);

    const auto localLights = createBuffer(m_localLights.data(),
        message.localLightCount * sizeof(LocalLight), 0x10);

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
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(4, geometryDescs);
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(5, materials);
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(6, instanceDescs);
    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(7, localLights);
    getUnderlyingGraphicsCommandList()->SetComputeRootDescriptorTable(8, m_descriptorHeap.getGpuHandle(m_uavId));

    const auto rayGenShaderTable = createBuffer(
        m_rayGenShaderTable.data(), static_cast<uint32_t>(m_rayGenShaderTable.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    const auto missShaderTable = createBuffer(
        m_missShaderTable.data(), static_cast<uint32_t>(m_missShaderTable.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    const auto hitGroupShaderTable = createBuffer(
        m_hitGroupShaderTable.data(), static_cast<uint32_t>(m_hitGroupShaderTable.size()), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

    D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc{};

    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = rayGenShaderTable;
    dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.MissShaderTable.StartAddress = missShaderTable;
    dispatchRaysDesc.MissShaderTable.SizeInBytes = m_missShaderTable.size();
    dispatchRaysDesc.MissShaderTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.HitGroupTable.StartAddress = hitGroupShaderTable;
    dispatchRaysDesc.HitGroupTable.SizeInBytes = m_hitGroupShaderTable.size();
    dispatchRaysDesc.HitGroupTable.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    dispatchRaysDesc.Width = m_upscaler->getWidth();
    dispatchRaysDesc.Height = m_upscaler->getHeight();
    dispatchRaysDesc.Depth = 1;

    PIX_BEGIN_EVENT("PrimaryRayGeneration");
    getGraphicsCommandList().commitBarriers();
    m_properties->SetPipelineStackSize(m_primaryStackSize);
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);

    getGraphicsCommandList().uavBarrier(m_depthTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_motionVectorsTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture0->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture1->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture2->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture3->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture4->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture5->GetResource());
    getGraphicsCommandList().uavBarrier(m_gBufferTexture6->GetResource());
    getGraphicsCommandList().uavBarrier(m_linearDepthTexture->GetResource());
    getGraphicsCommandList().commitBarriers();
    PIX_END_EVENT();

    PIX_BEGIN_EVENT("ShadowRayGeneration");
    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress += 2 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_properties->SetPipelineStackSize(m_shadowStackSize);
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);
    PIX_END_EVENT();

    PIX_BEGIN_EVENT("GIRayGeneration");
    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress += 2 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_properties->SetPipelineStackSize(m_giStackSize);
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);
    PIX_END_EVENT();

    PIX_BEGIN_EVENT("ReflectionRayGeneration");
    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress += 2 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_properties->SetPipelineStackSize(m_reflectionStackSize);
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);
    PIX_END_EVENT();

    PIX_BEGIN_EVENT("RefractionRayGeneration");
    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress += 2 * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_properties->SetPipelineStackSize(m_refractionStackSize);
    getUnderlyingGraphicsCommandList()->DispatchRays(&dispatchRaysDesc);
    PIX_END_EVENT();

    getGraphicsCommandList().uavBarrier(m_shadowTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_reservoirTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_giTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_reflectionTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_refractionTexture->GetResource());
    getGraphicsCommandList().uavBarrier(m_specularHitDistanceTexture->GetResource());
    getGraphicsCommandList().commitBarriers();

    resolveAndDispatchUpscaler(message);
    copyToRenderTargetAndDepthStencil(message);
}

void RaytracingDevice::procMsgCreateMaterial()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateMaterial>();

    if (m_materials.size() <= message.materialId)
        m_materials.resize(message.materialId + 1);

    auto& material = m_materials[message.materialId];

    material.shaderType = message.shaderType;
    material.flags = message.flags;

    const std::pair<const MsgCreateMaterial::Texture&, uint32_t&> textures[] =
    {
        { message.diffuseTexture,       material.diffuseTexture       },
        { message.diffuseTexture2,      material.diffuseTexture2      },
        { message.specularTexture,      material.specularTexture      },
        { message.specularTexture2,     material.specularTexture2     },
        { message.glossTexture,         material.glossTexture         },
        { message.glossTexture2,        material.glossTexture2        },
        { message.normalTexture,        material.normalTexture        },
        { message.normalTexture2,       material.normalTexture2       },
        { message.reflectionTexture,    material.reflectionTexture    },
        { message.opacityTexture,       material.opacityTexture       },
        { message.displacementTexture,  material.displacementTexture  },
        { message.levelTexture,         material.levelTexture         },
    };

    D3D12_SAMPLER_DESC samplerDesc{};
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    if (m_anisotropicFiltering > 0 &&
        message.shaderType != SHADER_TYPE_WATER_ADD &&
        message.shaderType != SHADER_TYPE_WATER_MUL &&
        message.shaderType != SHADER_TYPE_WATER_OPACITY)
    {
        samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        samplerDesc.MaxAnisotropy = m_anisotropicFiltering;
    }
    else
    {
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }

    for (const auto& [srcTexture, dstTexture] : textures)
    {
        dstTexture = NULL;

        if (srcTexture.id != NULL)
        {
            samplerDesc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeU);
            samplerDesc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeV);

            auto& sampler = m_samplers[XXH3_64bits(&samplerDesc, sizeof(samplerDesc))];
            if (!sampler)
            {
                sampler = m_samplerDescriptorHeap.allocate();

                m_device->CreateSampler(&samplerDesc,
                    m_samplerDescriptorHeap.getCpuHandle(sampler));
            }

            dstTexture = m_textures[srcTexture.id].srvIndex;
            dstTexture |= sampler << 20;
            dstTexture |= srcTexture.texCoordIndex << 30;
        }
    }

    assert((sizeof(Material) - offsetof(Material, texCoordOffsets)) == sizeof(MsgCreateMaterial) - offsetof(MsgCreateMaterial, texCoordOffsets));

    memcpy(material.texCoordOffsets, message.texCoordOffsets, sizeof(Material) - offsetof(Material, texCoordOffsets));
}

void RaytracingDevice::procMsgBuildBottomLevelAccelStruct()
{
    const auto& message = m_messageReceiver.getMessage<MsgBuildBottomLevelAccelStruct>();

    m_pendingBuilds.push_back(message.bottomLevelAccelStructId);
}

void RaytracingDevice::procMsgComputePose()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputePose>();

    PIX_EVENT();

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

    getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(0,
        createBuffer(message.data, message.nodeCount * sizeof(float[4][4]), 0x10));

    auto geometryDesc = reinterpret_cast<const MsgComputePose::GeometryDesc*>(
        message.data + message.nodeCount * sizeof(float[4][4]));

    auto nodePalette = reinterpret_cast<const uint32_t*>(geometryDesc + message.geometryCount);

    const auto destVertexBuffer = m_vertexBuffers[message.vertexBufferId].allocation->GetResource();
    auto destVirtualAddress = destVertexBuffer->GetGPUVirtualAddress();

    for (size_t i = 0; i < message.geometryCount; i++)
    {
        struct
        {
            uint32_t vertexCount;
            uint32_t vertexStride;
            uint32_t normalOffset;
            uint32_t tangentOffset;
            uint32_t binormalOffset;
            uint32_t blendWeightOffset;
            uint32_t blendIndicesOffset;
            uint32_t nodeCount;
        } cbGeometryDesc =
        {
            geometryDesc->vertexCount,
            geometryDesc->vertexStride,
            geometryDesc->normalOffset,
            geometryDesc->tangentOffset,
            geometryDesc->binormalOffset,
            geometryDesc->blendWeightOffset,
            geometryDesc->blendIndicesOffset,
            geometryDesc->nodeCount
        };

        getUnderlyingGraphicsCommandList()->SetComputeRootConstantBufferView(1,
            createBuffer(&cbGeometryDesc, sizeof(cbGeometryDesc), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(2,
            createBuffer(nodePalette, geometryDesc->nodeCount * sizeof(uint32_t), alignof(uint32_t)));

        getUnderlyingGraphicsCommandList()->SetComputeRootShaderResourceView(3,
            m_vertexBuffers[geometryDesc->vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress());

        getUnderlyingGraphicsCommandList()->SetComputeRootUnorderedAccessView(4, destVirtualAddress);

        getUnderlyingGraphicsCommandList()->Dispatch((geometryDesc->vertexCount + 63) / 64, 1, 1);

        destVirtualAddress += geometryDesc->vertexCount * (geometryDesc->vertexStride + 0xC); // Extra 12 bytes for previous position
        nodePalette += geometryDesc->nodeCount;
        ++geometryDesc;
    }
    m_pendingPoses.push_back(message.vertexBufferId);
}

void RaytracingDevice::procMsgRenderSky()
{
    const auto& message = m_messageReceiver.getMessage<MsgRenderSky>();

    PIX_EVENT();

    getGraphicsCommandList().transitionBarrier(m_skyTexture.Get(), 
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    getGraphicsCommandList().commitBarriers();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.pRootSignature = m_skyRootSignature.Get();
    pipelineDesc.VS.pShaderBytecode = SkyVertexShader;
    pipelineDesc.VS.BytecodeLength = sizeof(SkyVertexShader);
    pipelineDesc.PS.pShaderBytecode = SkyPixelShader;
    pipelineDesc.PS.BytecodeLength = sizeof(SkyPixelShader);
    pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    pipelineDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    pipelineDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    pipelineDesc.SampleMask = ~0;
    pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pipelineDesc.RasterizerState.DepthClipEnable = FALSE;
    pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
    pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineDesc.NumRenderTargets = 1;
    pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    pipelineDesc.SampleDesc.Count = 1;

    D3D12_SAMPLER_DESC samplerDesc{};
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

    if (m_anisotropicFiltering > 0)
    {
        samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        samplerDesc.MaxAnisotropy = m_anisotropicFiltering;
    }
    else
    {
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    }

    uint32_t samplerId = NULL;

    const auto renderTarget = m_rtvDescriptorHeap.getCpuHandle(m_skyRtvId);
    float clearValue[4]{};

    constexpr D3D12_VIEWPORT viewport = { 0, 0, CUBE_MAP_RESOLUTION, CUBE_MAP_RESOLUTION, 0.0f, 1.0f };
    constexpr D3D12_RECT scissorRect = { 0, 0, CUBE_MAP_RESOLUTION, CUBE_MAP_RESOLUTION };

    getUnderlyingGraphicsCommandList()->SetGraphicsRootSignature(m_skyRootSignature.Get());
    getUnderlyingGraphicsCommandList()->OMSetRenderTargets(1, &renderTarget, FALSE, nullptr);
    getUnderlyingGraphicsCommandList()->ClearRenderTargetView(renderTarget, clearValue, 0, nullptr);
    getUnderlyingGraphicsCommandList()->RSSetViewports(1, &viewport);
    getUnderlyingGraphicsCommandList()->RSSetScissorRects(1, &scissorRect);
    getUnderlyingGraphicsCommandList()->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    GlobalsSB globalsSB;
    globalsSB.backgroundScale = message.backgroundScale;

    auto geometryDesc = reinterpret_cast<const MsgRenderSky::GeometryDesc*>(message.data);
    auto lastGeometryDesc = reinterpret_cast<const MsgRenderSky::GeometryDesc*>(message.data + message.dataSize);

    while (geometryDesc < lastGeometryDesc)
    {
        globalsSB.enableAlphaTest = (geometryDesc->flags & GEOMETRY_FLAG_PUNCH_THROUGH) != 0;
        globalsSB.enableVertexColor = geometryDesc->enableVertexColor;

        std::pair<const MsgCreateMaterial::Texture&, uint32_t&> textures[] =
        {
            { geometryDesc->diffuseTexture, globalsSB.diffuseTextureId },
            { geometryDesc->alphaTexture, globalsSB.alphaTextureId },
            { geometryDesc->emissionTexture, globalsSB.emissionTextureId }
        };

        for (auto& [srcTexture, dstTexture] : textures)
        {
            if (srcTexture.id != NULL)
            {
                dstTexture = m_textures[srcTexture.id].srvIndex;

                if (samplerDesc.AddressU != srcTexture.addressModeU ||
                    samplerDesc.AddressV != srcTexture.addressModeV)
                {
                    samplerDesc.AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeU);
                    samplerDesc.AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(srcTexture.addressModeV);

                    auto& sampler = m_samplers[XXH3_64bits(&samplerDesc, sizeof(samplerDesc))];
                    if (!sampler)
                    {
                        sampler = m_samplerDescriptorHeap.allocate();
                        m_device->CreateSampler(&samplerDesc, m_samplerDescriptorHeap.getCpuHandle(sampler));
                    }

                    samplerId = sampler;
                }

                dstTexture |= samplerId << 20;
                dstTexture |= srcTexture.texCoordIndex << 30;
            }
            else
            {
                dstTexture = 0;
            }
        }

        memcpy(globalsSB.diffuse, geometryDesc->diffuse, sizeof(globalsSB.diffuse));
        memcpy(globalsSB.ambient, geometryDesc->ambient, sizeof(globalsSB.ambient));

        const auto& vertexDeclaration = m_vertexDeclarations[geometryDesc->vertexDeclarationId];

        const BOOL blendEnable = (geometryDesc->flags & GEOMETRY_FLAG_TRANSPARENT) != 0;
        const auto destBlend = geometryDesc->isAdditive ? D3D12_BLEND_ONE : D3D12_BLEND_INV_SRC_ALPHA;

        if (pipelineDesc.InputLayout.pInputElementDescs != vertexDeclaration.inputElements.get() ||
            pipelineDesc.InputLayout.NumElements != vertexDeclaration.inputElementsSize ||
            pipelineDesc.BlendState.RenderTarget[0].BlendEnable != blendEnable ||
            pipelineDesc.BlendState.RenderTarget[0].DestBlend != destBlend)
        {
            pipelineDesc.InputLayout.pInputElementDescs = vertexDeclaration.inputElements.get();
            pipelineDesc.InputLayout.NumElements = vertexDeclaration.inputElementsSize;
            pipelineDesc.BlendState.RenderTarget[0].BlendEnable = blendEnable;
            pipelineDesc.BlendState.RenderTarget[0].DestBlend = destBlend;

            auto& pipeline = m_pipelines[XXH3_64bits(&pipelineDesc, sizeof(pipelineDesc))];
            if (!pipeline)
                m_pipelineLibrary.createGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(pipeline.GetAddressOf()));

            getUnderlyingGraphicsCommandList()->SetPipelineState(pipeline.Get());
            m_curPipeline = pipeline.Get();
        }

        getUnderlyingGraphicsCommandList()->SetGraphicsRootConstantBufferView(0,
            createBuffer(&globalsSB, sizeof(GlobalsSB), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
        vertexBufferView.BufferLocation = m_vertexBuffers[geometryDesc->vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = geometryDesc->vertexStride;
        vertexBufferView.SizeInBytes = geometryDesc->vertexStride * geometryDesc->vertexCount;

        getUnderlyingGraphicsCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

        D3D12_INDEX_BUFFER_VIEW indexBufferView{};
        indexBufferView.BufferLocation = m_indexBuffers[geometryDesc->indexBufferId].allocation->GetResource()->GetGPUVirtualAddress();
        indexBufferView.SizeInBytes = geometryDesc->indexCount * 2;
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            
        getUnderlyingGraphicsCommandList()->IASetIndexBuffer(&indexBufferView);

        getUnderlyingGraphicsCommandList()->DrawIndexedInstanced(geometryDesc->indexCount, 6, 0, 0, 0);

        ++geometryDesc;
    }

    getGraphicsCommandList().transitionBarrier(m_skyTexture.Get(), 
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    m_dirtyFlags |=
        DIRTY_FLAG_ROOT_SIGNATURE |
        DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL |
        DIRTY_FLAG_PIPELINE_DESC |
        DIRTY_FLAG_GLOBALS_PS |
        DIRTY_FLAG_GLOBALS_VS |
        DIRTY_FLAG_VIEWPORT |
        DIRTY_FLAG_SCISSOR_RECT |
        DIRTY_FLAG_VERTEX_BUFFER_VIEWS |
        DIRTY_FLAG_INDEX_BUFFER_VIEW |
        DIRTY_FLAG_PRIMITIVE_TOPOLOGY;
}

void RaytracingDevice::procMsgCreateLocalLight()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateLocalLight>();

    if (m_localLights.size() <= message.localLightId)
        m_localLights.resize(message.localLightId + 1);

    auto& localLight = m_localLights[message.localLightId];

    memcpy(localLight.position, message.position, sizeof(localLight.position));
    localLight.inRange = message.inRange;
    memcpy(localLight.color, message.color, sizeof(localLight.color));
    localLight.outRange = message.outRange;
}

void RaytracingDevice::procMsgComputeSmoothNormal()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputeSmoothNormal>();

    m_smoothNormalCommands.push_back(
    {
        message.indexBufferId,
        message.vertexStride,
        message.vertexCount,
        message.vertexOffset,
        message.normalOffset,
        message.vertexBufferId,
        message.adjacencyBufferId
    });
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
    case MsgBuildBottomLevelAccelStruct::s_id: procMsgBuildBottomLevelAccelStruct(); break;
    case MsgComputePose::s_id: procMsgComputePose(); break;
    case MsgRenderSky::s_id: procMsgRenderSky(); break;
    case MsgCreateLocalLight::s_id: procMsgCreateLocalLight(); break;
    case MsgComputeSmoothNormal::s_id: procMsgComputeSmoothNormal(); break;
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
    if (m_device == nullptr)
        return;

    CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[1];
    descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, s_uavTextureNum, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 raytracingRootParams[9];
    raytracingRootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[3].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[4].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[5].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[6].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[7].InitAsShaderResourceView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[8].InitAsDescriptorTable(_countof(descriptorRanges), descriptorRanges);

    D3D12_STATIC_SAMPLER_DESC raytracingStaticSamplers[1]{};
    raytracingStaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    raytracingStaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    RootSignature::create(
        m_device.Get(),
        raytracingRootParams,
        _countof(raytracingRootParams),
        raytracingStaticSamplers,
        _countof(raytracingStaticSamplers),
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED,
        m_raytracingRootSignature, "raytracing");

    CD3DX12_STATE_OBJECT_DESC stateObject(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    std::unique_ptr<uint8_t[]> dxilLibraryHolder;

    FILE* file = fopen("DXILLibrary.cso", "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        const long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        dxilLibraryHolder = std::make_unique<uint8_t[]>(fileSize);
        fread(dxilLibraryHolder.get(), 1, fileSize, file);
        fclose(file);

        const uint8_t* dxilLibraryPtr = dxilLibraryHolder.get();

        while (dxilLibraryPtr < dxilLibraryHolder.get() + fileSize)
        {
            const CD3DX12_SHADER_BYTECODE dxilLibrary(dxilLibraryPtr + sizeof(uint32_t),
                *reinterpret_cast<const uint32_t*>(dxilLibraryPtr));

            const auto dxilLibrarySubobject = stateObject.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            dxilLibrarySubobject->SetDXILLibrary(&dxilLibrary);

            dxilLibraryPtr += sizeof(uint32_t) + dxilLibrary.BytecodeLength;
        }
    }

    CD3DX12_DXIL_LIBRARY_SUBOBJECT dxilLibrarySubobject(stateObject);
    const CD3DX12_SHADER_BYTECODE dxilLibrary(DXILLibrary, sizeof(DXILLibrary));
    dxilLibrarySubobject.SetDXILLibrary(&dxilLibrary);

    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject(stateObject);
    shaderConfigSubobject.Config(sizeof(float) * 12, sizeof(float) * 2);

    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT pipelineConfigSubobject(stateObject);
    pipelineConfigSubobject.Config(1);

    CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT rootSignatureSubobject(stateObject);
    rootSignatureSubobject.SetRootSignature(m_raytracingRootSignature.Get());

    HRESULT hr = m_device->CreateStateObject(stateObject, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_stateObject != nullptr);

    hr = m_stateObject.As(std::addressof(m_properties));
    assert(SUCCEEDED(hr) && m_properties != nullptr);

    m_primaryStackSize = m_properties->GetShaderStackSize(L"PrimaryRayGeneration");
    m_shadowStackSize = m_properties->GetShaderStackSize(L"ShadowRayGeneration");
    m_giStackSize = m_properties->GetShaderStackSize(L"GIRayGeneration");
    m_reflectionStackSize = m_properties->GetShaderStackSize(L"ReflectionRayGeneration");
    m_refractionStackSize = m_properties->GetShaderStackSize(L"RefractionRayGeneration");

    const wchar_t* rayGenShaderTable[] =
    {
        L"PrimaryRayGeneration",
        L"ShadowRayGeneration",
        L"GIRayGeneration",
        L"ReflectionRayGeneration",
        L"RefractionRayGeneration"
    };

    m_rayGenShaderTable.resize(_countof(rayGenShaderTable) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    for (size_t i = 0; i < _countof(rayGenShaderTable); i++)
        memcpy(&m_rayGenShaderTable[i * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT], m_properties->GetShaderIdentifier(rayGenShaderTable[i]), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    const wchar_t* missShaderTable[] =
    {
        L"PrimaryMiss",
        L"GIMiss",
        L"SecondaryMiss"
    };

    m_missShaderTable.resize(_countof(missShaderTable) * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    for (size_t i = 0; i < _countof(missShaderTable); i++)
        memcpy(&m_missShaderTable[i * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], m_properties->GetShaderIdentifier(missShaderTable[i]), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    for (size_t i = 0; i < _countof(m_hitGroups); i++)
        m_hitGroups[i] = m_properties->GetShaderIdentifier(s_shaderHitGroups[i]);

    for (auto& scratchBuffer : m_scratchBuffers)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

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

    CD3DX12_DESCRIPTOR_RANGE1 copyDescriptorRanges[1];
    copyDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 copyRootParams[1];
    copyRootParams[0].InitAsDescriptorTable(_countof(copyDescriptorRanges), copyDescriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

    D3D12_STATIC_SAMPLER_DESC copyStaticSamplers[1]{};
    copyStaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    copyStaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    copyStaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    copyStaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    RootSignature::create(
        m_device.Get(),
        copyRootParams,
        _countof(copyRootParams),
        copyStaticSamplers,
        _countof(copyStaticSamplers),
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS,
        m_copyTextureRootSignature, "copy");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC copyPipelineDesc{};
    copyPipelineDesc.pRootSignature = m_copyTextureRootSignature.Get();
    copyPipelineDesc.VS.pShaderBytecode = CopyTextureVertexShader;
    copyPipelineDesc.VS.BytecodeLength = sizeof(CopyTextureVertexShader);
    copyPipelineDesc.PS.pShaderBytecode = CopyTexturePixelShader;
    copyPipelineDesc.PS.BytecodeLength = sizeof(CopyTexturePixelShader);
    copyPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    copyPipelineDesc.SampleMask = ~0;
    copyPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    copyPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    copyPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    copyPipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    copyPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    copyPipelineDesc.NumRenderTargets = 1;
    copyPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    copyPipelineDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    copyPipelineDesc.SampleDesc.Count = 1;

    m_pipelineLibrary.createGraphicsPipelineState(&copyPipelineDesc, IID_PPV_ARGS(m_copyTexturePipeline.GetAddressOf()));

    CD3DX12_ROOT_PARAMETER1 poseRootParams[5];
    poseRootParams[0].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    poseRootParams[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    poseRootParams[2].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    poseRootParams[3].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    poseRootParams[4].InitAsUnorderedAccessView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);

    RootSignature::create(
        m_device.Get(),
        poseRootParams,
        _countof(poseRootParams),
        nullptr,
        0,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
        m_poseRootSignature, "pose");

    D3D12_COMPUTE_PIPELINE_STATE_DESC posePipelineDesc{};
    posePipelineDesc.pRootSignature = m_poseRootSignature.Get();
    posePipelineDesc.CS.pShaderBytecode = PoseComputeShader;
    posePipelineDesc.CS.BytecodeLength = sizeof(PoseComputeShader);

    m_pipelineLibrary.createComputePipelineState(&posePipelineDesc, IID_PPV_ARGS(m_posePipeline.GetAddressOf()));

    for (size_t i = 0; i < _countof(m_upscalerOverride); i++)
        m_upscalerOverride[i] = static_cast<UpscalerType>(i);

    IniFile iniFile;
    if (iniFile.read("GenerationsRaytracing.ini"))
    {
        m_qualityMode = static_cast<QualityMode>(
            iniFile.get<uint32_t>("Mod", "QualityMode", static_cast<uint32_t>(QualityMode::Balanced)));

        const auto upscalerType = static_cast<UpscalerType>(iniFile.get<uint32_t>("Mod", "Upscaler", static_cast<uint32_t>(UpscalerType::FSR2)));

        if (upscalerType == UpscalerType::DLSS || upscalerType == UpscalerType::DLSSD)
        {
            std::unique_ptr<DLSS> upscaler;

            if (upscalerType == UpscalerType::DLSSD)
                upscaler = std::make_unique<DLSSD>(*this);
            else
                upscaler = std::make_unique<DLSS>(*this);

            if (!upscaler->valid())
            {
                MessageBox(nullptr,
                    TEXT("An NVIDIA GPU with up to date drivers is required for DLSS. FSR2 will be used instead as a fallback."),
                    TEXT("Generations Raytracing"),
                    MB_ICONERROR);

                m_upscalerOverride[static_cast<size_t>(upscalerType)] = UpscalerType::FSR2;
            }
            else
            {
                m_upscaler = std::move(upscaler);
            }
        }

        m_anisotropicFiltering = iniFile.get<uint32_t>("Mod", "AnisotropicFiltering", 0);
    }

    if (m_upscaler == nullptr)
        m_upscaler = std::make_unique<FSR2>(*this);

    D3D12_COMPUTE_PIPELINE_STATE_DESC resolvePipelineDesc{};
    resolvePipelineDesc.pRootSignature = m_raytracingRootSignature.Get();
    resolvePipelineDesc.CS.pShaderBytecode = ResolveComputeShader;
    resolvePipelineDesc.CS.BytecodeLength = sizeof(ResolveComputeShader);

    m_pipelineLibrary.createComputePipelineState(&resolvePipelineDesc, IID_PPV_ARGS(m_resolvePipeline.GetAddressOf()));

    CD3DX12_ROOT_PARAMETER1 skyRootParams[1];
    skyRootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_ALL);

    RootSignature::create(
        m_device.Get(),
        skyRootParams,
        _countof(skyRootParams),
        nullptr,
        0,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED |
        D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED,
        m_skyRootSignature, "sky");

    const CD3DX12_HEAP_PROPERTIES skyHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    const auto skyResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        CUBE_MAP_RESOLUTION, CUBE_MAP_RESOLUTION, 6, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    float color[4]{};
    const CD3DX12_CLEAR_VALUE clearValue(DXGI_FORMAT_R16G16B16A16_FLOAT, color);

    hr = m_device->CreateCommittedResource(
        &skyHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &skyResourceDesc,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_PPV_ARGS(m_skyTexture.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_skyTexture != nullptr);

    D3D12_SHADER_RESOURCE_VIEW_DESC skySrvDesc{};
    skySrvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    skySrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    skySrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    skySrvDesc.TextureCube.MipLevels = 1;

    m_globalsRT.skyTextureId = m_descriptorHeap.allocate();
    m_device->CreateShaderResourceView(m_skyTexture.Get(),
        &skySrvDesc, m_descriptorHeap.getCpuHandle(m_globalsRT.skyTextureId));

    m_skyRtvId = m_rtvDescriptorHeap.allocate();

    D3D12_RENDER_TARGET_VIEW_DESC skyRtvDesc{};
    skyRtvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    skyRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    skyRtvDesc.Texture2DArray.FirstArraySlice = 0;
    skyRtvDesc.Texture2DArray.ArraySize = 6;

    m_device->CreateRenderTargetView(m_skyTexture.Get(),
        &skyRtvDesc, m_rtvDescriptorHeap.getCpuHandle(m_skyRtvId));

    CD3DX12_ROOT_PARAMETER1 smoothNormalRootParams[3];
    smoothNormalRootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    smoothNormalRootParams[1].InitAsUnorderedAccessView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    smoothNormalRootParams[2].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

    RootSignature::create(
        m_device.Get(),
        smoothNormalRootParams,
        _countof(smoothNormalRootParams),
        nullptr,
        0,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED,
        m_smoothNormalRootSignature,
        "smooth_normal");

    D3D12_COMPUTE_PIPELINE_STATE_DESC smoothNormalPipelineDesc{};
    smoothNormalPipelineDesc.pRootSignature = m_smoothNormalRootSignature.Get();
    smoothNormalPipelineDesc.CS.pShaderBytecode = SmoothNormalComputeShader;
    smoothNormalPipelineDesc.CS.BytecodeLength = sizeof(SmoothNormalComputeShader);

    m_pipelineLibrary.createComputePipelineState(&smoothNormalPipelineDesc, 
        IID_PPV_ARGS(m_smoothNormalPipeline.GetAddressOf()));
}

static void waitForQueueOnExit(CommandQueue& queue)
{
    if (queue.getUnderlyingQueue() != nullptr)
    {
        const uint64_t fenceValue = queue.getNextFenceValue();
        queue.signal(fenceValue);
        queue.wait(fenceValue);
    }
}

RaytracingDevice::~RaytracingDevice()
{
    // Wait for GPU operations to finish to exit cleanly
    waitForQueueOnExit(m_copyQueue);
    waitForQueueOnExit(m_graphicsQueue);
}
