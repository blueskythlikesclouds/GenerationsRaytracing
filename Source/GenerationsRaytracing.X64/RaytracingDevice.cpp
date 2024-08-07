#include "RaytracingDevice.h"

#include "DXILLibrary.h"
#include "CopyTextureVertexShader.h"
#include "CopyTexturePixelShader.h"
#include "DebugView.h"
#include "DLSS.h"
#include "DLSSD.h"
#include "EnvironmentColor.h"
#include "EnvironmentMode.h"
#include "FSR3.h"
#include "GBufferData.h"
#include "GeometryFlags.h"
#include "MaterialFlags.h"
#include "AlignmentUtil.h"
#include "Message.h"
#include "RootSignature.h"
#include "PoseComputeShader.h"
#include "ResolveComputeShader.h"
#include "SkyVertexShader.h"
#include "SkyPixelShader.h"
#include "PIXEvent.h"
#include "ShaderTable.h"
#include "SmoothNormalComputeShader.h"
#include "ResolveTransparencyComputeShader.h"
#include "ShaderType.h"
#include "DXILLibrarySER.h"
#include "Im3dLinesVertexShader.h"
#include "Im3dLinesPixelShader.h"
#include "Im3dPointsVertexShader.h"
#include "Im3dPointsPixelShader.h"
#include "Im3dTrianglesVertexShader.h"
#include "Im3dTrianglesPixelShader.h"
#include "ReservoirComputeShader.h"
#include "CopyHdrTexturePixelShader.h"
#include "GrassInstancerComputeShader.h"

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
    if ((id + count) == m_geometryDescs.size())
    {
        m_geometryDescs.resize(id);
        m_hitGroupShaderTable.resize(id * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * HIT_GROUP_NUM);
    }
    else
    {
        m_freeIndices.emplace(id, m_freeCounts.emplace(count, id));
    }
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO RaytracingDevice::buildAccelStruct(SubAllocation& allocation,
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO preBuildInfo{};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&accelStructDesc.Inputs, &preBuildInfo);

    if (allocation.blockAllocation == nullptr || allocation.byteSize < preBuildInfo.ResultDataMaxSizeInBytes)
    {
        if (allocation.blockAllocation != nullptr)
            m_tempBottomLevelAccelStructs[m_frame].push_back(std::move(allocation));

        allocation = m_bottomLevelAccelStructAllocator.allocate(m_allocator.Get(),
            static_cast<uint32_t>(preBuildInfo.ResultDataMaxSizeInBytes));
    }

    accelStructDesc.DestAccelerationStructureData = allocation.getGpuVA();

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
            accelStructDesc.ScratchAccelerationStructureData = 
                m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

            m_scratchBufferOffset = alignUp<uint32_t>(m_scratchBufferOffset +
                static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes), D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
        }

        auto& commandList = getGraphicsCommandList();

        commandList.getUnderlyingCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
        commandList.uavBarrier(allocation.blockAllocation->GetResource());
    }

    return preBuildInfo;
}

void RaytracingDevice::buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct)
{
    uint32_t scratchBufferSize;

    if (bottomLevelAccelStruct.desc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE)
    {
        bottomLevelAccelStruct.desc.SourceAccelerationStructureData = bottomLevelAccelStruct.allocation.getGpuVA();
        scratchBufferSize = bottomLevelAccelStruct.updateScratchBufferSize;
    }
    else
    {
        bottomLevelAccelStruct.desc.SourceAccelerationStructureData = NULL;
        scratchBufferSize = bottomLevelAccelStruct.scratchBufferSize;
    }

    // Create separate resource if it does not fit to the pool
    if (m_scratchBufferOffset + scratchBufferSize > SCRATCH_BUFFER_SIZE)
    {
        auto& scratchBuffer = m_tempBuffers[m_frame].emplace_back();

        createBuffer(
            D3D12_HEAP_TYPE_DEFAULT,
            scratchBufferSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON,
            scratchBuffer);

        bottomLevelAccelStruct.desc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();
    }
    else
    {
        bottomLevelAccelStruct.desc.ScratchAccelerationStructureData =
            m_scratchBuffers[m_frame]->GetResource()->GetGPUVirtualAddress() + m_scratchBufferOffset;

        m_scratchBufferOffset = alignUp<uint32_t>(m_scratchBufferOffset + scratchBufferSize,
            D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
    }

    auto& commandList = bottomLevelAccelStruct.asyncBuild ? getComputeCommandList() : getGraphicsCommandList();

    if (bottomLevelAccelStruct.asyncBuild)
        commandList.open();

    commandList.getUnderlyingCommandList()->BuildRaytracingAccelerationStructure(&bottomLevelAccelStruct.desc, 0, nullptr);

    if (!bottomLevelAccelStruct.asyncBuild)
        commandList.uavBarrier(bottomLevelAccelStruct.allocation.blockAllocation->GetResource());

    bottomLevelAccelStruct.asyncBuild = false;
}

void RaytracingDevice::handlePendingBottomLevelAccelStructBuilds()
{
    PIX_EVENT();

    auto& commandList = getGraphicsCommandList();

    for (const auto pendingPose : m_pendingPoses)
        commandList.uavBarrier(m_vertexBuffers[pendingPose].allocation->GetResource());

    commandList.commitBarriers();

    for (const auto pendingBuild : m_pendingBuilds)
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[pendingBuild];
        buildBottomLevelAccelStruct(bottomLevelAccelStruct);
    }

    m_pendingPoses.clear();
    m_pendingBuilds.clear();
}

void RaytracingDevice::handlePendingSmoothNormalCommands()
{
    if (m_smoothNormalCommands.empty())
        return;

    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_EVENT();

    if (m_curRootSignature != m_smoothNormalRootSignature.Get())
    {
        underlyingCommandList->SetComputeRootSignature(m_smoothNormalRootSignature.Get());
        m_curRootSignature = m_smoothNormalRootSignature.Get();
    }
    if (m_curPipeline != m_smoothNormalPipeline.Get())
    {
        underlyingCommandList->SetPipelineState(m_smoothNormalPipeline.Get());
        m_curPipeline = m_smoothNormalPipeline.Get();
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    }

    for (const auto& cmd : m_smoothNormalCommands)
    {
        struct
        {
            uint32_t indexBufferId;
            uint32_t indexOffset;
            uint32_t vertexStride;
            uint32_t vertexCount;
            uint32_t normalOffset;
            uint32_t isMirrored;
        } geometryDesc = 
        {
            m_indexBuffers[cmd.indexBufferId].srvIndex,
            cmd.indexOffset,
            cmd.vertexStride,
            cmd.vertexCount,
            cmd.normalOffset,
            cmd.isMirrored
        };

        underlyingCommandList->SetComputeRootConstantBufferView(0,
            createBuffer(&geometryDesc, sizeof(geometryDesc), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        const auto& vertexBuffer = m_vertexBuffers[cmd.vertexBufferId];

        underlyingCommandList->SetComputeRootUnorderedAccessView(1,
            vertexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + cmd.vertexOffset);

        underlyingCommandList->SetComputeRootShaderResourceView(2,
            m_indexBuffers[cmd.adjacencyBufferId].allocation->GetResource()->GetGPUVirtualAddress());

        underlyingCommandList->Dispatch((cmd.vertexCount + 63) / 64, 1, 1);

        commandList.uavBarrier(vertexBuffer.allocation->GetResource());
    }

    m_smoothNormalCommands.clear();
}

void RaytracingDevice::writeHitGroupShaderTable(size_t geometryIndex, size_t shaderType, bool constTexCoord)
{
    uint8_t* hitGroupTable = &m_hitGroupShaderTable[geometryIndex * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * HIT_GROUP_NUM];
    void* const* hitGroups = &m_hitGroups[shaderType * (_countof(m_hitGroups) / SHADER_TYPE_MAX)];

    for (size_t i = 0; i < HIT_GROUP_NUM; i++)
    {
        size_t index;

        switch (i)
        {
        case HIT_GROUP_PRIMARY:
            index = constTexCoord ? 1 : 0;
            break;

        case HIT_GROUP_PRIMARY_TRANSPARENT:
            index = constTexCoord ? 3 : 2;
            break;

        case HIT_GROUP_SECONDARY:
            index = 4;
            break;

        default:
            assert(false);
            break;
        }

        memcpy(&hitGroupTable[i * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], hitGroups[index], D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
}

static int32_t getJitterPhaseCount(int32_t renderWidth, int32_t displayWidth)
{
    const float basePhaseCount = 8.0f;
    const int32_t jitterPhaseCount = int32_t(basePhaseCount * pow((float(displayWidth) / renderWidth), 2.0f));
    return jitterPhaseCount;
}

static float halton(int32_t index, int32_t base)
{
    float f = 1.0f, result = 0.0f;

    for (int32_t currentIndex = index; currentIndex > 0;) {

        f /= (float)base;
        result = result + f * (float)(currentIndex % base);
        currentIndex = (uint32_t)(floorf((float)(currentIndex) / (float)(base)));
    }

    return result;
}

static void getJitterOffset(float* outX, float* outY, int32_t index, int32_t phaseCount)
{
    const float x = halton((index % phaseCount) + 1, 2) - 0.5f;
    const float y = halton((index % phaseCount) + 1, 3) - 0.5f;

    *outX = x;
    *outY = y;
}

D3D12_GPU_VIRTUAL_ADDRESS RaytracingDevice::createGlobalsRT(const MsgTraceRays& message)
{
    if (message.resetAccumulation || message.debugView != DEBUG_VIEW_NONE || 
        (m_upscaler == nullptr && memcmp(m_globalsRT.prevProj, m_globalsVS.floatConstants, 0x80) != 0))
    {
        m_globalsRT.currentFrame = 0;
    }

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

    getJitterOffset(&m_globalsRT.pixelJitterX, &m_globalsRT.pixelJitterY, static_cast<int32_t>(m_globalsRT.currentFrame),
        getJitterPhaseCount(static_cast<int32_t>(m_renderWidth), static_cast<int32_t>(m_width)));

    m_globalsRT.internalResolutionWidth = m_renderWidth;
    m_globalsRT.internalResolutionHeight = m_renderHeight;
    m_globalsRT.randomSeed = m_globalsRT.internalResolutionWidth * m_globalsRT.internalResolutionHeight * m_globalsRT.currentFrame;
    m_globalsRT.localLightCount = static_cast<uint32_t>(m_localLights.size());
    m_globalsRT.diffusePower = message.diffusePower;
    m_globalsRT.lightPower = message.lightPower;
    m_globalsRT.emissivePower = message.emissivePower;
    m_globalsRT.skyPower = message.skyPower;
    memcpy(m_globalsRT.backgroundColor, message.backgroundColor, sizeof(m_globalsRT.backgroundColor));
    m_globalsRT.useSkyTexture = message.useSkyTexture;
    m_globalsRT.adaptionLuminanceTextureId = m_textures[message.adaptionLuminanceTextureId].srvIndex;
    m_globalsRT.middleGray = message.middleGray;
    m_globalsRT.skyInRoughReflection = message.skyInRoughReflection;
    m_globalsRT.enableAccumulation = m_globalsRT.currentFrame > 0 && m_upscaler == nullptr;
    m_globalsRT.envBrdfTextureId = m_textures[message.envBrdfTextureId].srvIndex;

    getGraphicsCommandList().transitionBarrier(m_textures[message.adaptionLuminanceTextureId].allocation->GetResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    const auto globalsRT = createBuffer(&m_globalsRT, sizeof(GlobalsRT), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    memcpy(m_globalsRT.prevProj, m_globalsVS.floatConstants, 0x80);
    ++m_globalsRT.currentFrame;

    return globalsRT;
}

void RaytracingDevice::createTopLevelAccelStructs()
{
    handlePendingBottomLevelAccelStructBuilds();
    handlePendingSmoothNormalCommands();

    getGraphicsCommandList().commitBarriers();

    PIX_EVENT();

    for (auto& topLevelAccelStruct : m_topLevelAccelStructs)
    {
        if (!topLevelAccelStruct.instanceDescs.empty())
        {
            D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC accelStructDesc{};
            accelStructDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            accelStructDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            accelStructDesc.Inputs.NumDescs = static_cast<UINT>(topLevelAccelStruct.instanceDescs.size());
            accelStructDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
            accelStructDesc.Inputs.InstanceDescs = createBuffer(topLevelAccelStruct.instanceDescs.data(),
                static_cast<uint32_t>(topLevelAccelStruct.instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)), D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

            buildAccelStruct(topLevelAccelStruct.allocation, accelStructDesc, true);
        }
    }
}

static constexpr size_t s_textureCount = 28;

void RaytracingDevice::createRaytracingTextures()
{
    bool shouldCreateTextures = false;

    if (m_upscaler != nullptr)
    {
        m_upscaler->init({ *this, m_width, m_height, 
            m_qualityMode == QualityMode::Auto ? getAutoQualityMode(m_height) : m_qualityMode });
        
        shouldCreateTextures = m_renderWidth != m_upscaler->getWidth() || m_renderHeight != m_upscaler->getHeight() ||
            (m_colorTexture != nullptr && m_colorTexture->GetResource()->GetDesc().Format != DXGI_FORMAT_R16G16B16A16_FLOAT);

        m_renderWidth = m_upscaler->getWidth();
        m_renderHeight = m_upscaler->getHeight();

        setDescriptorHeaps();
        m_curRootSignature = nullptr;
    }
    else
    {
        shouldCreateTextures = m_renderWidth != m_width || m_renderHeight != m_height ||
            (m_colorTexture != nullptr && m_colorTexture->GetResource()->GetDesc().Format != DXGI_FORMAT_R32G32B32A32_FLOAT);

        m_renderWidth = m_width;
        m_renderHeight = m_height;
    }

    m_globalsRT.currentFrame = 0;

    if (shouldCreateTextures)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

        auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_UNKNOWN,
            0, 0, 1, 1, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        const auto colorTextureFormat = m_upscaler != nullptr ? 
            DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R32G32B32A32_FLOAT;

        struct
        {
            uint16_t depthOrArraySize{};
            DXGI_FORMAT format{};
            ComPtr<D3D12MA::Allocation>& allocation;
            const wchar_t* name{};
            uint32_t width{};
            uint32_t height{};
            D3D12_RESOURCE_FLAGS flags{};
        }
        const textureDescs[] =
        {
            { 1, colorTextureFormat, m_colorTexture, L"Color Texture", 0, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_outputTexture, L"Output Texture", m_width, m_height },
            { 1, DXGI_FORMAT_R32_FLOAT, m_depthTexture, L"Depth Texture" },
            { 1, DXGI_FORMAT_R16G16_FLOAT, m_motionVectorsTexture, L"Motion Vectors Texture" },
            { 1, DXGI_FORMAT_R32_FLOAT, m_exposureTexture, L"Exposure Texture", 1, 1 },

            { 1, DXGI_FORMAT_R8_UINT, m_layerNumTexture, L"Layer Num Texture" },

            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32G32B32A32_FLOAT, m_gBufferTexture0, L"Geometry Buffer Texture 0" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32_UINT, m_gBufferTexture1, L"Geometry Buffer Texture 1" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32_UINT, m_gBufferTexture2, L"Geometry Buffer Texture 2" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32_UINT, m_gBufferTexture3, L"Geometry Buffer Texture 3" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R8G8_UNORM, m_gBufferTexture4, L"Geometry Buffer Texture 4" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R16G16_UNORM, m_gBufferTexture5, L"Geometry Buffer Texture 5" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32_UINT, m_gBufferTexture6, L"Geometry Buffer Texture 6" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R32_UINT, m_gBufferTexture7, L"Geometry Buffer Texture 7" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R16G16B16A16_FLOAT, m_gBufferTexture8, L"Geometry Buffer Texture 8" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R8G8_UNORM, m_gBufferTexture9, L"Geometry Buffer Texture 9" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R16G16_UNORM, m_gBufferTexture10, L"Geometry Buffer Texture 10" },

            { 1, DXGI_FORMAT_R32G32B32A32_UINT, m_reservoirTexture, L"Reservoir Texture" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R16G16B16A16_FLOAT, m_giTexture, L"GI Texture" },
            { GBUFFER_LAYER_NUM, DXGI_FORMAT_R16G16B16A16_FLOAT, m_reflectionTexture, L"Reflection Texture" },

            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_diffuseAlbedoTexture, L"Diffuse Albedo Texture", 0, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_specularAlbedoTexture, L"Specular Albedo Texture", 0, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET },
            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_normalsRoughnessTexture, L"Normals Roughness Texture" },
            { 1, DXGI_FORMAT_R16_FLOAT, m_linearDepthTexture, L"Linear Depth Texture" },
            { 1, DXGI_FORMAT_R16_FLOAT, m_specularHitDistanceTexture, L"Specular Hit Distance Texture" },

            { 1, colorTextureFormat, m_colorBeforeTransparencyTexture, L"Color Before Transparency Texture" },
            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_diffuseAlbedoBeforeTransparencyTexture, L"Diffuse Albedo Before Transparency Texture" },
            { 1, DXGI_FORMAT_R16G16B16A16_FLOAT, m_specularAlbedoBeforeTransparencyTexture, L"Specular Albedo Before Transparency Texture" },
        };

        static_assert(_countof(textureDescs) == s_textureCount);

        for (const auto& textureDesc : textureDescs)
        {
            resourceDesc.Width = textureDesc.width > 0 ? textureDesc.width : m_renderWidth;
            resourceDesc.Height = textureDesc.height > 0 ? textureDesc.height : m_renderHeight;
            resourceDesc.DepthOrArraySize = textureDesc.depthOrArraySize;
            resourceDesc.Format = textureDesc.format;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | textureDesc.flags;

            const HRESULT hr = m_allocator->CreateResource(
                &allocDesc,
                &resourceDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                textureDesc.allocation.ReleaseAndGetAddressOf(),
                IID_ID3D12Resource,
                nullptr);

            assert(SUCCEEDED(hr) && textureDesc.allocation != nullptr);

#ifdef _DEBUG
            textureDesc.allocation->GetResource()->SetName(textureDesc.name);
#endif
        }

        assert(m_uavId != NULL && m_srvId != NULL);

        auto uavHandle = m_descriptorHeap.getCpuHandle(m_uavId);
        auto srvHandle = m_descriptorHeap.getCpuHandle(m_srvId);

        for (const auto& textureDesc : textureDescs)
        {
            auto resource = textureDesc.allocation->GetResource();

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = textureDesc.format;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = textureDesc.format;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (textureDesc.depthOrArraySize > 1)
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.ArraySize = textureDesc.depthOrArraySize;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MipLevels = 1;
                srvDesc.Texture2DArray.ArraySize = textureDesc.depthOrArraySize;
            }
            else
            {
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
            }

            m_device->CreateUnorderedAccessView(
                resource,
                nullptr,
                &uavDesc,
                uavHandle);

            m_device->CreateShaderResourceView(
                resource,
                &srvDesc,
                srvHandle);

            if (m_exposureTexture == textureDesc.allocation)
                m_exposureTextureId = m_descriptorHeap.getIndex(srvHandle);

            uavHandle.ptr += m_descriptorHeap.getIncrementSize();
            srvHandle.ptr += m_descriptorHeap.getIncrementSize();
        }

        m_device->CreateRenderTargetView(m_colorTexture->GetResource(), nullptr, m_rtvDescriptorHeap.getCpuHandle(m_colorRtvId));
        m_device->CreateRenderTargetView(m_diffuseAlbedoTexture->GetResource(), nullptr, m_rtvDescriptorHeap.getCpuHandle(m_diffuseAlbedoRtvId));
        m_device->CreateRenderTargetView(m_specularAlbedoTexture->GetResource(), nullptr, m_rtvDescriptorHeap.getCpuHandle(m_specularAlbedoRtvId));

        if (m_frameGenerator != nullptr)
        {
            m_frameGenerator->init(
                {
                    *this,
                    m_width,
                    m_height,
                    m_renderWidth,
                    m_renderHeight,
                    m_copyHdrTexturePipeline != nullptr
                });
        }
    }
}

void RaytracingDevice::dispatchResolver(const MsgTraceRays& message)
{
    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_BEGIN_EVENT("Resolve");

    underlyingCommandList->SetPipelineState(m_resolvePipeline.Get());

    underlyingCommandList->Dispatch(
        (m_renderWidth + 7) / 8,
        (m_renderHeight + 7) / 8,
        1);

    commandList.uavBarrier(m_normalsRoughnessTexture->GetResource());
    commandList.uavBarrier(m_specularHitDistanceTexture->GetResource());

    commandList.transitionBarriers(
        { 
            m_colorBeforeTransparencyTexture->GetResource(),
            m_diffuseAlbedoBeforeTransparencyTexture->GetResource(),
            m_specularAlbedoBeforeTransparencyTexture->GetResource()
        }, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList.commitBarriers();

    PIX_END_EVENT();

    PIX_BEGIN_EVENT("Resolve Transparency");

    underlyingCommandList->SetPipelineState(m_resolveTransparencyPipeline.Get());

    underlyingCommandList->Dispatch(
        (m_renderWidth + 7) / 8,
        (m_renderHeight + 7) / 8,
        1);

    PIX_END_EVENT();

    storeForAccumulator();
}

void RaytracingDevice::copyToRenderTargetAndDepthStencil(const MsgDispatchUpscaler& message)
{
    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_EVENT();

    const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f };
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

    struct
    {
        uint32_t width;
        uint32_t height;
        uint32_t debugView;
    } globals =
    {
        m_renderWidth,
        m_renderHeight,
        m_upscaler != nullptr || message.debugView != DEBUG_VIEW_NONE ? message.debugView : DEBUG_VIEW_COLOR
    };

    underlyingCommandList->OMSetRenderTargets(1, m_renderTargetViews, FALSE, &m_depthStencilView);
    underlyingCommandList->SetGraphicsRootSignature(m_copyTextureRootSignature.Get());
    underlyingCommandList->SetGraphicsRoot32BitConstants(0, 3, &globals, 0);
    underlyingCommandList->SetGraphicsRootDescriptorTable(1, m_descriptorHeap.getGpuHandle(m_srvId));
    underlyingCommandList->SetPipelineState(m_copyTexturePipeline.Get());
    underlyingCommandList->RSSetViewports(1, &viewport);
    underlyingCommandList->RSSetScissorRects(1, &scissorRect);
    underlyingCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList.transitionBarrier(m_outputTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList.transitionBarrier(m_depthTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    commandList.transitionBarrier(m_renderTargetTextures[0],
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList.transitionBarrier(m_depthStencilTexture,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    commandList.commitBarriers();

    underlyingCommandList->DrawInstanced(6, 1, 0, 0);

    m_dirtyFlags |=
        DIRTY_FLAG_ROOT_SIGNATURE |
        DIRTY_FLAG_PIPELINE_DESC |
        DIRTY_FLAG_GLOBALS_PS |
        DIRTY_FLAG_GLOBALS_VS |
        DIRTY_FLAG_VIEWPORT |
        DIRTY_FLAG_SCISSOR_RECT |
        DIRTY_FLAG_PRIMITIVE_TOPOLOGY;

    m_dirtyFlags &= ~DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

    restoreForAccumulator();
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

        auto& vertexBuffer = m_vertexBuffers[geometryDesc.vertexBufferId];

        assert(vertexBuffer.byteSize - geometryDesc.vertexOffset >= geometryDesc.vertexCount * geometryDesc.vertexStride);
        assert((geometryDesc.vertexOffset % 4) == 0);

        if (geometryDesc.indexBufferId != NULL)
        {
            assert((geometryDesc.indexCount % 3) == 0);
            assert(m_indexBuffers[geometryDesc.indexBufferId].byteSize - geometryDesc.indexOffset * sizeof(uint16_t) >= geometryDesc.indexCount * sizeof(uint16_t));
        }

        // BVH GeometryDesc
        {
            auto& dstGeometryDesc = bottomLevelAccelStruct.geometryDescs[i];

            dstGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            if (!(geometryDesc.flags & (GEOMETRY_FLAG_TRANSPARENT | GEOMETRY_FLAG_PUNCH_THROUGH)))
                dstGeometryDesc.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

            auto& triangles = dstGeometryDesc.Triangles;
            memset(&triangles, 0, sizeof(triangles));

            triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            triangles.VertexCount = geometryDesc.vertexCount;
            triangles.VertexBuffer.StartAddress = vertexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc.vertexOffset;
            triangles.VertexBuffer.StrideInBytes = geometryDesc.vertexStride;

            if (geometryDesc.indexBufferId != NULL)
            {
                auto& indexBuffer = m_indexBuffers[geometryDesc.indexBufferId];

                triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
                triangles.IndexCount = geometryDesc.indexCount;
                triangles.IndexBuffer = indexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc.indexOffset * sizeof(uint16_t);
            }
        }

        // GPU GeometryDesc
        {
            auto& dstGeometryDesc = m_geometryDescs[bottomLevelAccelStruct.geometryId + i];

            dstGeometryDesc.flags = geometryDesc.flags;
            dstGeometryDesc.indexBufferId = geometryDesc.indexBufferId != NULL ? m_indexBuffers[geometryDesc.indexBufferId].srvIndex : NULL;
            dstGeometryDesc.indexOffset = geometryDesc.indexOffset;
            dstGeometryDesc.vertexBufferId = vertexBuffer.srvIndex;
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
        writeHitGroupShaderTable(bottomLevelAccelStruct.geometryId + i, material.shaderType - 1, (material.flags & MATERIAL_FLAG_CONST_TEX_COORD) != 0);

        if (material.flags & MATERIAL_FLAG_DOUBLE_SIDED)
            bottomLevelAccelStruct.instanceFlags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    }

    bottomLevelAccelStruct.desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

    if (message.allowUpdate)
        bottomLevelAccelStruct.desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    if (message.allowCompaction)
        bottomLevelAccelStruct.desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;

    if (message.preferFastBuild)
        bottomLevelAccelStruct.desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    else
        bottomLevelAccelStruct.desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    bottomLevelAccelStruct.desc.Inputs.NumDescs = static_cast<UINT>(bottomLevelAccelStruct.geometryDescs.size());
    bottomLevelAccelStruct.desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelAccelStruct.desc.Inputs.pGeometryDescs = bottomLevelAccelStruct.geometryDescs.data();

    auto preBuildInfo = buildAccelStruct(bottomLevelAccelStruct.allocation, bottomLevelAccelStruct.desc, false);
    bottomLevelAccelStruct.scratchBufferSize = static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes);
    bottomLevelAccelStruct.updateScratchBufferSize = static_cast<uint32_t>(preBuildInfo.UpdateScratchDataSizeInBytes);
    bottomLevelAccelStruct.asyncBuild = message.asyncBuild;

    m_pendingBuilds.emplace_back(message.bottomLevelAccelStructId);
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case RaytracingResourceType::BottomLevelAccelStruct:
    {
        auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.resourceId];

        m_tempBottomLevelAccelStructs[m_frame].emplace_back(std::move(bottomLevelAccelStruct.allocation));

        if (bottomLevelAccelStruct.geometryCount != 0)
            m_tempGeometryRanges[m_frame].emplace_back(bottomLevelAccelStruct.geometryId, bottomLevelAccelStruct.geometryCount);

        bottomLevelAccelStruct = {};
        break;
    }

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

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];
    if (!bottomLevelAccelStruct.asyncBuild)
    {
        auto& instanceDesc = m_topLevelAccelStructs[message.instanceType].instanceDescs.emplace_back();
        auto& alsoInstanceDesc = m_topLevelAccelStructs[message.instanceType].alsoInstanceDescs.emplace_back();

        memcpy(instanceDesc.Transform, message.transform, sizeof(instanceDesc.Transform));
        memcpy(alsoInstanceDesc.prevTransform, message.prevTransform, sizeof(alsoInstanceDesc.prevTransform));
        memcpy(alsoInstanceDesc.headTransform, message.headTransform, sizeof(alsoInstanceDesc.headTransform));
        alsoInstanceDesc.playableParam = message.playableParam;
        alsoInstanceDesc.chrPlayableMenuParam = message.chrPlayableMenuParam;
        alsoInstanceDesc.forceAlphaColor = message.forceAlphaColor;
        alsoInstanceDesc.edgeEmissionParam = message.edgeEmissionParam;

        if (message.dataSize > 0)
        {
            auto& [geometryId, geometryCount] = m_tempGeometryRanges[m_frame].emplace_back();

            geometryId = allocateGeometryDescs(bottomLevelAccelStruct.geometryCount);
            geometryCount = bottomLevelAccelStruct.geometryCount;

            auto& geometryDesc = m_geometryDescs[geometryId];

            for (size_t i = 0; i < geometryCount; i++)
            {
                auto& geometry = m_geometryDescs[geometryId + i];
                geometry = m_geometryDescs[bottomLevelAccelStruct.geometryId + i];

                auto curId = reinterpret_cast<const uint32_t*>(message.data);
                const auto lastId = reinterpret_cast<const uint32_t*>(message.data + message.dataSize);

                while (curId < lastId)
                {
                    if ((*curId) == geometry.materialId)
                        geometry.materialId = *(curId + 1);

                    curId += 2;
                }

                const auto& material = m_materials[geometry.materialId];
                writeHitGroupShaderTable(geometryId + i, material.shaderType - 1, (material.flags & MATERIAL_FLAG_CONST_TEX_COORD) != 0);
            }

            instanceDesc.InstanceID = geometryId;
        }
        else
        {
            instanceDesc.InstanceID = bottomLevelAccelStruct.geometryId;
        }

        instanceDesc.InstanceMask = message.instanceMask;
        instanceDesc.InstanceContributionToHitGroupIndex = instanceDesc.InstanceID * HIT_GROUP_NUM;
        instanceDesc.Flags = bottomLevelAccelStruct.instanceFlags;

        if (message.isMirrored && !(instanceDesc.Flags & D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE))
            instanceDesc.Flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

        instanceDesc.AccelerationStructure = bottomLevelAccelStruct.allocation.getGpuVA();
    }
}

void RaytracingDevice::procMsgTraceRays()
{
    const auto& message = m_messageReceiver.getMessage<MsgTraceRays>();

    createTopLevelAccelStructs();

    if (m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].instanceDescs.empty() &&
        m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].instanceDescs.empty())
    {
        return;
    }

    const auto upscalerType = static_cast<UpscalerType>(message.upscaler);
    const auto qualityMode = static_cast<QualityMode>(message.qualityMode);

    const bool resolutionMismatch = m_width != message.width || m_height != message.height;
    const bool upscalerMismatch = (m_upscaler != nullptr ? m_upscaler->getType() : UpscalerType::None) != m_upscalerOverride[message.upscaler];
    const bool qualityModeMismatch = upscalerType != UpscalerType::None && m_qualityMode != qualityMode;

    if (resolutionMismatch || upscalerMismatch || qualityModeMismatch)
    {
        m_width = message.width;
        m_height = message.height;

        m_graphicsQueue.getFence()->SetEventOnCompletion(m_fenceValues[(m_frame - 1) % NUM_FRAMES], nullptr);

        if (upscalerMismatch)
        {
            if (m_ngx == nullptr && (upscalerType == UpscalerType::DLSS || upscalerType == UpscalerType::DLSSD))
                m_ngx = std::make_unique<NGX>(*this);

            std::unique_ptr<DLSS> upscaler;

            if (m_ngx != nullptr && m_ngx->valid())
            {
                if (upscalerType == UpscalerType::DLSS && DLSS::valid(*m_ngx))
                    upscaler = std::make_unique<DLSS>(*m_ngx);

                else if (upscalerType == UpscalerType::DLSSD && DLSSD::valid(*m_ngx))
                    upscaler = std::make_unique<DLSSD>(*m_ngx);
            }

            if (upscaler == nullptr)
            {
                if (upscalerType != UpscalerType::None)
                {
                    m_upscaler = std::make_unique<FSR3>(*this);
                    m_upscalerOverride[message.upscaler] = UpscalerType::FSR3;
                }
                else
                {
                    m_upscaler = nullptr;
                }
            }
            else
            {
                m_upscaler = std::move(upscaler);
            }
        }

        if (qualityModeMismatch)
            m_qualityMode = qualityMode;

        createRaytracingTextures();
    }

    const auto globalsVS = createBuffer(&m_globalsVS,
        sizeof(m_globalsVS), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsPS = createBuffer(&m_globalsPS, 
        offsetof(GlobalsPS, textureIndices), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    const auto globalsRT = createGlobalsRT(message);

    const auto topLevelAccelStruct = !m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].instanceDescs.empty() ? 
        m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].allocation.getGpuVA() : NULL;

    const auto topLevelAccelStructTransparent = !m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].instanceDescs.empty() ?
        m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].allocation.getGpuVA() : NULL;

    const auto geometryDescs = createBuffer(m_geometryDescs.data(), 
        static_cast<uint32_t>(m_geometryDescs.size() * sizeof(GeometryDesc)), 0x10);

    const auto materials = createBuffer(m_materials.data(),
        static_cast<uint32_t>(m_materials.size() * sizeof(Material)), 0x10);

    const auto instanceDescs = createBuffer(m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].alsoInstanceDescs.data(),
        static_cast<uint32_t>(m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].alsoInstanceDescs.size() * sizeof(InstanceDesc)), 0x10);

    const auto instanceDescsTransparent = createBuffer(m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].alsoInstanceDescs.data(),
        static_cast<uint32_t>(m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].alsoInstanceDescs.size() * sizeof(InstanceDesc)), 0x10);

    const auto localLights = createBuffer(m_localLights.data(),
        static_cast<uint32_t>(m_localLights.size() * sizeof(LocalLight)), 0x10);

    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    if (m_curRootSignature != m_raytracingRootSignature.Get())
    {
        underlyingCommandList->SetComputeRootSignature(m_raytracingRootSignature.Get());
        m_curRootSignature = m_raytracingRootSignature.Get();
    }
    underlyingCommandList->SetPipelineState1(m_stateObject.Get());

    underlyingCommandList->SetComputeRootConstantBufferView(0, globalsVS);
    underlyingCommandList->SetComputeRootConstantBufferView(1, globalsPS);
    underlyingCommandList->SetComputeRootConstantBufferView(2, globalsRT);
    underlyingCommandList->SetComputeRootShaderResourceView(3, topLevelAccelStruct);
    underlyingCommandList->SetComputeRootShaderResourceView(4, topLevelAccelStructTransparent);
    underlyingCommandList->SetComputeRootShaderResourceView(5, geometryDescs);
    underlyingCommandList->SetComputeRootShaderResourceView(6, materials);
    underlyingCommandList->SetComputeRootShaderResourceView(7, instanceDescs);
    underlyingCommandList->SetComputeRootShaderResourceView(8, instanceDescsTransparent);
    underlyingCommandList->SetComputeRootShaderResourceView(9, localLights);
    underlyingCommandList->SetComputeRootDescriptorTable(10, m_descriptorHeap.getGpuHandle(m_uavId));
    underlyingCommandList->SetComputeRootDescriptorTable(11, m_descriptorHeap.getGpuHandle(m_srvId));

    if (m_serSupported)
        underlyingCommandList->SetComputeRootDescriptorTable(12, m_descriptorHeap.getGpuHandle(m_serUavId));

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
    dispatchRaysDesc.Width = m_renderWidth;
    dispatchRaysDesc.Height = m_renderHeight;
    dispatchRaysDesc.Depth = 1;

    PIX_BEGIN_EVENT("PrimaryRayGeneration");
    commandList.commitBarriers();

    m_properties->SetPipelineStackSize(m_primaryStackSize);
    underlyingCommandList->DispatchRays(&dispatchRaysDesc);

    commandList.transitionBarriers(
        {
            m_motionVectorsTexture->GetResource(),
            m_layerNumTexture->GetResource(),
            m_gBufferTexture0->GetResource(),
            m_gBufferTexture1->GetResource(),
            m_gBufferTexture2->GetResource(),
            m_gBufferTexture3->GetResource(),
            m_gBufferTexture4->GetResource(),
            m_gBufferTexture5->GetResource(),
            m_gBufferTexture6->GetResource(),
            m_gBufferTexture7->GetResource(),
            m_gBufferTexture8->GetResource(),
            m_gBufferTexture9->GetResource(),
            m_gBufferTexture10->GetResource(),
        }, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList.commitBarriers();

    PIX_END_EVENT();

    PIX_BEGIN_EVENT("SecondaryRayGeneration");
    m_properties->SetPipelineStackSize(m_secondaryStackSize);
    dispatchRaysDesc.RayGenerationShaderRecord.StartAddress += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    underlyingCommandList->DispatchRays(&dispatchRaysDesc);
    PIX_END_EVENT();

    if (!m_localLights.empty())
    {
        PIX_BEGIN_EVENT("Reservoir");

        underlyingCommandList->SetPipelineState(m_reservoirPipeline.Get());

        underlyingCommandList->Dispatch(
            (m_renderWidth + 7) / 8,
            (m_renderHeight + 7) / 8,
            1);

        PIX_END_EVENT();
    }

    commandList.transitionBarriers(
        {
            m_depthTexture->GetResource(),
            m_reservoirTexture->GetResource(),
            m_giTexture->GetResource(),
            m_reflectionTexture->GetResource(),
            m_linearDepthTexture->GetResource()
        }, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    commandList.commitBarriers();

    dispatchResolver(message);
    prepareForDispatchUpscaler(message);
}

void RaytracingDevice::prepareForDispatchUpscaler(const MsgTraceRays& message)
{
    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_BEGIN_EVENT("Prepare For Upscaler");

    commandList.transitionBarrier(m_depthTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList.transitionBarrier(m_depthStencilTexture,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList.commitBarriers();

    const CD3DX12_TEXTURE_COPY_LOCATION depthSrcLocation(m_depthTexture->GetResource());
    const CD3DX12_TEXTURE_COPY_LOCATION depthDstLocation(m_depthStencilTexture);

    underlyingCommandList->CopyTextureRegion(
        &depthDstLocation,
        0,
        0,
        0,
        &depthSrcLocation,
        nullptr);

    commandList.transitionBarrier(m_colorTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList.transitionBarrier(m_diffuseAlbedoTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList.transitionBarrier(m_specularAlbedoTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList.transitionBarrier(m_depthStencilTexture,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);

    PIX_END_EVENT();

    m_renderTargetView = m_renderTargetViews[0];
    m_renderTargetFormat = m_pipelineDesc.RTVFormats[0];

    m_renderTargetViews[0] = m_rtvDescriptorHeap.getCpuHandle(m_colorRtvId);
    m_renderTargetViews[1] = m_rtvDescriptorHeap.getCpuHandle(m_diffuseAlbedoRtvId);
    m_renderTargetViews[2] = m_rtvDescriptorHeap.getCpuHandle(m_specularAlbedoRtvId);
    m_dirtyFlags |= DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL;

    m_pipelineDesc.RTVFormats[0] = m_colorTexture->GetResource()->GetDesc().Format;
    m_pipelineDesc.RTVFormats[1] = m_diffuseAlbedoTexture->GetResource()->GetDesc().Format;
    m_pipelineDesc.RTVFormats[2] = m_specularAlbedoTexture->GetResource()->GetDesc().Format;
    m_pipelineDesc.NumRenderTargets = 3;
    m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

    m_globalsVS.floatConstants[3][0] += 2.0f * m_globalsRT.pixelJitterX / static_cast<float>(m_renderWidth);
    m_globalsVS.floatConstants[3][1] += -2.0f * m_globalsRT.pixelJitterY / static_cast<float>(m_renderHeight);
    m_dirtyFlags |= DIRTY_FLAG_GLOBALS_VS;

    if (message.enableExposureTexture)
    {
        m_globalsPS.exposureTextureId = m_exposureTextureId;
        m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;
    }

    m_viewport.Width = static_cast<FLOAT>(m_renderWidth);
    m_viewport.Height = static_cast<FLOAT>(m_renderHeight);
    m_dirtyFlags |= DIRTY_FLAG_VIEWPORT;

    m_scissorRect.right = static_cast<LONG>(m_renderWidth);
    m_scissorRect.bottom = static_cast<LONG>(m_renderHeight);
    m_dirtyFlags |= DIRTY_FLAG_SCISSOR_RECT;

    const float mipLodBias = log2f(static_cast<float>(m_renderWidth) / static_cast<float>(m_width)) - 1.0f;

    for (auto& samplerDesc : m_samplerDescs)
        samplerDesc.MipLODBias = mipLodBias;

    m_samplerDescsFirst = 0;
    m_samplerDescsLast = _countof(m_samplerDescs) - 1;
}

void RaytracingDevice::storeForAccumulator()
{
    if (m_upscaler != nullptr)
        return;

    PIX_EVENT();

    auto& commandList = getGraphicsCommandList();

    commandList.transitionBarrier(m_colorTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList.transitionBarrier(m_colorBeforeTransparencyTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList.commitBarriers();

    commandList.getUnderlyingCommandList()->CopyResource(m_colorBeforeTransparencyTexture->GetResource(), m_colorTexture->GetResource());
}

void RaytracingDevice::restoreForAccumulator()
{
    if (m_upscaler != nullptr)
        return;

    PIX_EVENT();

    auto& commandList = getGraphicsCommandList();

    commandList.transitionBarrier(m_colorBeforeTransparencyTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

    commandList.transitionBarrier(m_colorTexture->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList.commitBarriers();

    commandList.getUnderlyingCommandList()->CopyResource(m_colorTexture->GetResource(), m_colorBeforeTransparencyTexture->GetResource());
}

void RaytracingDevice::procMsgDispatchUpscaler()
{
    const auto& message = m_messageReceiver.getMessage<MsgDispatchUpscaler>();

    if (m_topLevelAccelStructs[INSTANCE_TYPE_OPAQUE].instanceDescs.empty() &&
        m_topLevelAccelStructs[INSTANCE_TYPE_TRANSPARENT].instanceDescs.empty())
    {
        return;
    }
    
    bool shouldSetDescriptorHeaps = false;

    if (m_upscaler != nullptr && message.debugView == DEBUG_VIEW_NONE)
    {
        auto& commandList = getGraphicsCommandList();

        PIX_BEGIN_EVENT("Upscale");

        commandList.transitionBarriers(
            {
                m_colorTexture->GetResource(),
                m_depthTexture->GetResource(),
                m_exposureTexture->GetResource(),
                m_diffuseAlbedoTexture->GetResource(),
                m_specularAlbedoTexture->GetResource(),
                m_normalsRoughnessTexture->GetResource(),
                m_specularHitDistanceTexture->GetResource(),
            }, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList.commitBarriers();

        m_upscaler->dispatch(
            {
                *this,
                m_diffuseAlbedoTexture->GetResource(),
                m_specularAlbedoTexture->GetResource(),
                m_normalsRoughnessTexture->GetResource(),
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

        shouldSetDescriptorHeaps = true;
    }

    if (m_frameGenerator != nullptr)
    {
        m_frameGenerator->dispatch(
            {
                *this,
                m_width,
                m_height,
                m_globalsRT.currentFrame,
                m_renderWidth,
                m_renderHeight,
                m_depthTexture->GetResource(),
                m_motionVectorsTexture->GetResource(),
                m_globalsRT.pixelJitterX,
                m_globalsRT.pixelJitterY,
                message.resetAccumulation
            });

        shouldSetDescriptorHeaps = true;
    }

    if (shouldSetDescriptorHeaps)
        setDescriptorHeaps();

    m_renderTargetViews[0] = m_renderTargetView;
    copyToRenderTargetAndDepthStencil(message);

    m_renderTargetViews[1].ptr = NULL;
    m_renderTargetViews[2].ptr = NULL;

    m_pipelineDesc.RTVFormats[0] = m_renderTargetFormat;
    m_pipelineDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
    m_pipelineDesc.RTVFormats[2] = DXGI_FORMAT_UNKNOWN;
    m_pipelineDesc.NumRenderTargets = 1;
    m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;

    m_globalsVS.floatConstants[3][0] = 0.0f;
    m_globalsVS.floatConstants[3][1] = 0.0f;
    m_dirtyFlags |= DIRTY_FLAG_GLOBALS_VS;

    m_globalsPS.exposureTextureId = NULL;
    m_dirtyFlags |= DIRTY_FLAG_GLOBALS_PS;

    m_viewport.Width = static_cast<FLOAT>(m_width);
    m_viewport.Height = static_cast<FLOAT>(m_height);
    m_dirtyFlags |= DIRTY_FLAG_VIEWPORT;

    m_scissorRect.right = static_cast<LONG>(m_width);
    m_scissorRect.bottom = static_cast<LONG>(m_height);
    m_dirtyFlags |= DIRTY_FLAG_SCISSOR_RECT;

    for (auto& samplerDesc : m_samplerDescs)
        samplerDesc.MipLODBias = 0.0f;

    m_samplerDescsFirst = 0;
    m_samplerDescsLast = _countof(m_samplerDescs) - 1;
}

void RaytracingDevice::procMsgCreateMaterial()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateMaterial>();

    if (m_materials.size() <= message.materialId)
        m_materials.resize(message.materialId + 1);

    auto& material = m_materials[message.materialId];

    material.shaderType = message.shaderType + 1;
    material.flags = message.flags;
    memcpy(material.texCoordOffsets, message.texCoordOffsets, sizeof(material.texCoordOffsets));

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

    for (size_t i = 0; i < message.textureCount; i++)
    {
        auto& dstTexture = material.packedData[i];
        auto& srcTexture = message.textures[i];
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

    memcpy(&material.packedData[message.textureCount], message.parameters, message.parameterCount * sizeof(float));
}

void RaytracingDevice::procMsgBuildBottomLevelAccelStruct()
{
    const auto& message = m_messageReceiver.getMessage<MsgBuildBottomLevelAccelStruct>();

    m_pendingBuilds.push_back(message.bottomLevelAccelStructId);

    auto& bottomLevelAccelStruct = m_bottomLevelAccelStructs[message.bottomLevelAccelStructId];
    if (message.performUpdate)
        bottomLevelAccelStruct.desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
    else 
        bottomLevelAccelStruct.desc.Inputs.Flags &= ~D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
}

void RaytracingDevice::procMsgComputePose()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputePose>();

    const auto underlyingCommandList = getUnderlyingGraphicsCommandList();

    PIX_EVENT();

    if (m_curRootSignature != m_poseRootSignature.Get())
    {
        underlyingCommandList->SetComputeRootSignature(m_poseRootSignature.Get());
        m_curRootSignature = m_poseRootSignature.Get();
    }
    if (m_curPipeline != m_posePipeline.Get())
    {
        underlyingCommandList->SetPipelineState(m_posePipeline.Get());
        m_curPipeline = m_posePipeline.Get();
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    }

    underlyingCommandList->SetComputeRootShaderResourceView(0,
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
            uint32_t blendWeight1Offset;
            uint32_t blendIndices1Offset;
            uint32_t nodeCount;
            uint32_t visible;
        } cbGeometryDesc =
        {
            geometryDesc->vertexCount,
            geometryDesc->vertexStride,
            geometryDesc->normalOffset,
            geometryDesc->tangentOffset,
            geometryDesc->binormalOffset,
            geometryDesc->blendWeightOffset,
            geometryDesc->blendIndicesOffset,
            geometryDesc->blendWeight1Offset,
            geometryDesc->blendIndices1Offset,
            geometryDesc->nodeCount,
            geometryDesc->visible
        };

        underlyingCommandList->SetComputeRootConstantBufferView(1,
            createBuffer(&cbGeometryDesc, sizeof(cbGeometryDesc), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        underlyingCommandList->SetComputeRootShaderResourceView(2,
            createBuffer(nodePalette, geometryDesc->nodeCount * sizeof(uint32_t), alignof(uint32_t)));

        underlyingCommandList->SetComputeRootShaderResourceView(3,
            m_vertexBuffers[geometryDesc->vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc->vertexOffset);

        underlyingCommandList->SetComputeRootUnorderedAccessView(4, destVirtualAddress);

        underlyingCommandList->Dispatch((geometryDesc->vertexCount + 63) / 64, 1, 1);

        destVirtualAddress += geometryDesc->vertexCount * (geometryDesc->vertexStride + 0xC); // Extra 12 bytes for previous position
        nodePalette += geometryDesc->nodeCount;
        ++geometryDesc;
    }
    m_pendingPoses.push_back(message.vertexBufferId);
}

void RaytracingDevice::procMsgRenderSky()
{
    const auto& message = m_messageReceiver.getMessage<MsgRenderSky>();

    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_EVENT();

    commandList.transitionBarrier(m_skyTexture.Get(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList.commitBarriers();

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

    underlyingCommandList->SetGraphicsRootSignature(m_skyRootSignature.Get());
    underlyingCommandList->OMSetRenderTargets(1, &renderTarget, FALSE, nullptr);
    underlyingCommandList->ClearRenderTargetView(renderTarget, clearValue, 0, nullptr);
    underlyingCommandList->RSSetViewports(1, &viewport);
    underlyingCommandList->RSSetScissorRects(1, &scissorRect);
    underlyingCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

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

            underlyingCommandList->SetPipelineState(pipeline.Get());
            m_curPipeline = pipeline.Get();
        }

        underlyingCommandList->SetGraphicsRootConstantBufferView(0,
            createBuffer(&globalsSB, sizeof(GlobalsSB), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
        vertexBufferView.BufferLocation = m_vertexBuffers[geometryDesc->vertexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc->vertexOffset;
        vertexBufferView.StrideInBytes = geometryDesc->vertexStride;
        vertexBufferView.SizeInBytes = geometryDesc->vertexStride * geometryDesc->vertexCount;

        underlyingCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

        D3D12_INDEX_BUFFER_VIEW indexBufferView{};
        indexBufferView.BufferLocation = m_indexBuffers[geometryDesc->indexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + geometryDesc->indexOffset * 2;
        indexBufferView.SizeInBytes = geometryDesc->indexCount * 2;
        indexBufferView.Format = DXGI_FORMAT_R16_UINT;
            
        underlyingCommandList->IASetIndexBuffer(&indexBufferView);

        underlyingCommandList->DrawIndexedInstanced(geometryDesc->indexCount, 6, 0, 0, 0);

        ++geometryDesc;
    }

    commandList.transitionBarrier(m_skyTexture.Get(),
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

    auto& localLight = m_localLights.emplace_back();

    memcpy(localLight.position, message.position, sizeof(localLight.position));
    localLight.inRange = message.inRange;
    memcpy(localLight.color, message.color, sizeof(localLight.color));
    localLight.outRange = message.outRange;
    localLight.flags = message.castShadow ? LOCAL_LIGHT_FLAG_CAST_SHADOW : 0;
    localLight.flags |= message.enableBackfaceCulling ? LOCAL_LIGHT_FLAG_ENABLE_BACKFACE_CULLING : 0;
    localLight.shadowRange = message.shadowRange;
}

void RaytracingDevice::procMsgComputeSmoothNormal()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputeSmoothNormal>();

    m_smoothNormalCommands.push_back(
    {
        message.indexBufferId,
        message.indexOffset,
        message.vertexStride,
        message.vertexCount,
        message.vertexOffset,
        message.normalOffset,
        message.vertexBufferId,
        message.adjacencyBufferId,
        message.isMirrored
    });
}

void RaytracingDevice::procMsgDrawIm3d()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawIm3d>();

    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_EVENT();

    struct
    {
        float projection[4][4];
        float view[4][4];
        float viewportSize[2];
    } globals;

    memcpy(globals.projection, message.projection, sizeof(globals.projection));
    memcpy(globals.view, message.view, sizeof(globals.view));
    memcpy(globals.viewportSize, message.viewportSize, sizeof(globals.viewportSize));

    const D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f};
    const D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

    const auto& depthStencilTexture = m_textures[message.depthStencilId];
    const auto depthStencilView = m_dsvDescriptorHeap.getCpuHandle(depthStencilTexture.dsvIndex);

    underlyingCommandList->SetGraphicsRootSignature(m_im3dRootSignature.Get());
    underlyingCommandList->OMSetRenderTargets(1, m_renderTargetViews, FALSE, &depthStencilView);
    underlyingCommandList->SetGraphicsRootConstantBufferView(0, 
        createBuffer(&globals, sizeof(globals), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
    underlyingCommandList->RSSetViewports(1, &viewport);
    underlyingCommandList->RSSetScissorRects(1, &scissorRect);

    commandList.transitionBarrier(depthStencilTexture.allocation->GetResource(),
        D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ);

    commandList.commitBarriers();

    const auto vertexData = createBuffer(message.data, message.vertexSize, alignof(float));
    auto drawList = reinterpret_cast<const MsgDrawIm3d::DrawList*>(message.data + message.vertexSize);

    for (size_t i = 0; i < message.drawListCount; i++)
    {
        enum
        {
            DrawPrimitive_Triangles,
            DrawPrimitive_Lines,
            DrawPrimitive_Points,
        };

        D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        ID3D12PipelineState* pipelineState = nullptr;
        uint32_t primitiveCount = 0;

        switch (drawList->primitiveType)
        {
        case DrawPrimitive_Triangles:
            primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            pipelineState = m_im3dTrianglesPipeline.Get();
            primitiveCount = drawList->vertexCount / 3;
            break;

        case DrawPrimitive_Lines:
            primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            pipelineState = m_im3dLinesPipeline.Get();
            primitiveCount = drawList->vertexCount / 2;
            break;

        case DrawPrimitive_Points:
            primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            pipelineState = m_im3dPointsPipeline.Get();
            primitiveCount = drawList->vertexCount;
            break;

        default:
            assert(false);
            break;
        }

        underlyingCommandList->SetGraphicsRootShaderResourceView(1, vertexData + drawList->vertexOffset);

        if (m_curPipeline != pipelineState)
        {
            underlyingCommandList->SetPipelineState(pipelineState);
            m_curPipeline = pipelineState;
        }

        if (m_primitiveTopology != primitiveTopology)
        {
            underlyingCommandList->IASetPrimitiveTopology(primitiveTopology);
            m_primitiveTopology = primitiveTopology;
        }

        underlyingCommandList->DrawInstanced(
            drawList->primitiveType == DrawPrimitive_Triangles ? 3 : 4,
            primitiveCount,
            0,
            0);

        ++drawList;
    }

    m_dirtyFlags |=
        DIRTY_FLAG_ROOT_SIGNATURE |
        DIRTY_FLAG_PIPELINE_DESC |
        DIRTY_FLAG_PRIMITIVE_TOPOLOGY |
        DIRTY_FLAG_SCISSOR_RECT |
        DIRTY_FLAG_VIEWPORT;
}

void RaytracingDevice::procMsgComputeGrassInstancer()
{
    const auto& message = m_messageReceiver.getMessage<MsgComputeGrassInstancer>();

    auto& commandList = getGraphicsCommandList();
    const auto underlyingCommandList = commandList.getUnderlyingCommandList();

    PIX_EVENT();

    if (m_curRootSignature != m_grassInstancerRootSignature.Get())
    {
        underlyingCommandList->SetComputeRootSignature(m_grassInstancerRootSignature.Get());
        m_curRootSignature = m_grassInstancerRootSignature.Get();
    }
    if (m_curPipeline != m_grassInstancerPipeline.Get())
    {
        underlyingCommandList->SetPipelineState(m_grassInstancerPipeline.Get());
        m_curPipeline = m_grassInstancerPipeline.Get();
        m_dirtyFlags |= DIRTY_FLAG_PIPELINE_DESC;
    }

    underlyingCommandList->SetComputeRootShaderResourceView(0,
        createBuffer(message.instanceTypes, sizeof(message.instanceTypes), 0x10));

    underlyingCommandList->SetComputeRoot32BitConstant(1, *reinterpret_cast<const UINT*>(&message.misc), 0);

    const auto& instanceBuffer = m_vertexBuffers[message.instanceBufferId];
    const auto& vertexBuffer = m_vertexBuffers[message.vertexBufferId];

    auto lodDesc = reinterpret_cast<const MsgComputeGrassInstancer::LodDesc*>(message.data);

    for (size_t i = 0; i < message.dataSize / sizeof(MsgComputeGrassInstancer::LodDesc); i++)
    {
        underlyingCommandList->SetComputeRoot32BitConstant(2, lodDesc->instanceCount * 6, 0);

        underlyingCommandList->SetComputeRootShaderResourceView(3,
            instanceBuffer.allocation->GetResource()->GetGPUVirtualAddress() + lodDesc->instanceOffset);

        underlyingCommandList->SetComputeRootUnorderedAccessView(4,
            vertexBuffer.allocation->GetResource()->GetGPUVirtualAddress() + lodDesc->vertexOffset);

        underlyingCommandList->Dispatch((lodDesc->instanceCount * 6 + 63) / 64, 1, 1);

        ++lodDesc;
    }

    commandList.uavBarrier(vertexBuffer.allocation->GetResource());
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
    case MsgDispatchUpscaler::s_id: procMsgDispatchUpscaler(); break;
    case MsgDrawIm3d::s_id: procMsgDrawIm3d(); break;
    case MsgComputeGrassInstancer::s_id: procMsgComputeGrassInstancer(); break;
    default: return false;
    }

    return true;
}

void RaytracingDevice::releaseRaytracingResources()
{
    m_curRootSignature = nullptr;
    m_curPipeline = nullptr;
    m_scratchBufferOffset = 0;
    m_globalsPS.exposureTextureId = NULL;

    for (auto& subAllocation : m_tempBottomLevelAccelStructs[m_frame])
        m_bottomLevelAccelStructAllocator.free(subAllocation);

    m_bottomLevelAccelStructAllocator.freeBlocks(m_tempBuffers[m_frame]);
    m_tempBottomLevelAccelStructs[m_frame].clear();

    while (!m_materials.empty() && m_materials.back().shaderType == NULL)
        m_materials.pop_back();

    for (auto& topLevelAccelStruct : m_topLevelAccelStructs)
    {
        topLevelAccelStruct.instanceDescs.clear();
        topLevelAccelStruct.alsoInstanceDescs.clear();
    }

    for (auto& [geometryId, geometryCount] : m_tempGeometryRanges[m_frame])
        freeGeometryDescs(geometryId, geometryCount);

    m_tempGeometryRanges[m_frame].clear();

    m_localLights.clear();
}

static constexpr size_t s_serUavRegister = 0;

RaytracingDevice::RaytracingDevice(const IniFile& iniFile) : Device(iniFile)
{
    if (m_device == nullptr)
        return;

#if 0
    NvAPI_Status status = NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_device.Get(), NV_EXTN_OP_HIT_OBJECT_REORDER_THREAD, &m_serSupported);
    if (status == NVAPI_OK && m_serSupported)
    {
        NVAPI_D3D12_RAYTRACING_THREAD_REORDERING_CAPS featureCaps = NVAPI_D3D12_RAYTRACING_THREAD_REORDERING_CAP_NONE;
        status = NvAPI_D3D12_GetRaytracingCaps(
            m_device.Get(),
            NVAPI_D3D12_RAYTRACING_CAPS_TYPE_THREAD_REORDERING,
            &featureCaps,
            sizeof(featureCaps));

        if (status != NVAPI_OK || featureCaps == NVAPI_D3D12_RAYTRACING_THREAD_REORDERING_CAP_NONE)
            m_serSupported = false;
    }
#endif

    CD3DX12_DESCRIPTOR_RANGE1 uavDescriptorRanges[1];
    uavDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, s_textureCount, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_DESCRIPTOR_RANGE1 srvDescriptorRanges[1];
    srvDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, s_textureCount, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_DESCRIPTOR_RANGE1 serDescriptorRanges[1];
    serDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, s_serUavRegister, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

    CD3DX12_ROOT_PARAMETER1 raytracingRootParams[13];
    raytracingRootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[2].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[3].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
    raytracingRootParams[4].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
    raytracingRootParams[5].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[6].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[7].InitAsShaderResourceView(4, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[8].InitAsShaderResourceView(5, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[9].InitAsShaderResourceView(6, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    raytracingRootParams[10].InitAsDescriptorTable(_countof(uavDescriptorRanges), uavDescriptorRanges);
    raytracingRootParams[11].InitAsDescriptorTable(_countof(srvDescriptorRanges), srvDescriptorRanges);
    raytracingRootParams[12].InitAsDescriptorTable(_countof(serDescriptorRanges), serDescriptorRanges);

    D3D12_STATIC_SAMPLER_DESC raytracingStaticSamplers[1]{};
    raytracingStaticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    raytracingStaticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    raytracingStaticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    RootSignature::create(
        m_device.Get(),
        raytracingRootParams,
        m_serSupported ? _countof(raytracingRootParams) : (_countof(raytracingRootParams) - 1),
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

    CD3DX12_SHADER_BYTECODE dxilLibrary(DXILLibrary, sizeof(DXILLibrary));
    if (m_serSupported)
    {
        dxilLibrary = CD3DX12_SHADER_BYTECODE{ DXILLibrarySER, sizeof(DXILLibrarySER) };
        NvAPI_D3D12_SetNvShaderExtnSlotSpace(m_device.Get(), s_serUavRegister, 0);

        m_serUavId = m_descriptorHeap.allocate();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.StructureByteStride = 0x100;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        m_device->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, m_descriptorHeap.getCpuHandle(m_serUavId));
    }

    dxilLibrarySubobject.SetDXILLibrary(&dxilLibrary);

    CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT shaderConfigSubobject(stateObject);
    shaderConfigSubobject.Config(sizeof(float) * 15, sizeof(float) * 2);

    CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT pipelineConfigSubobject(stateObject);
    pipelineConfigSubobject.Config(1);

    CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT rootSignatureSubobject(stateObject);
    rootSignatureSubobject.SetRootSignature(m_raytracingRootSignature.Get());

    HRESULT hr = m_device->CreateStateObject(stateObject, IID_PPV_ARGS(m_stateObject.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_stateObject != nullptr);

    if (m_serSupported)
        NvAPI_D3D12_SetNvShaderExtnSlotSpace(m_device.Get(), 0xFFFFFFFF, 0);

    hr = m_stateObject.As(std::addressof(m_properties));
    assert(SUCCEEDED(hr) && m_properties != nullptr);

    const size_t primaryStackSize = m_properties->GetShaderStackSize(L"PrimaryRayGeneration");
    const size_t secondaryStackSize = m_properties->GetShaderStackSize(L"SecondaryRayGeneration");
    constexpr size_t s_hitGroupStride = _countof(m_hitGroups) / SHADER_TYPE_MAX;
    constexpr const wchar_t* s_shaderTypes[] = { L"::anyhit", L"::closesthit" };

    for (size_t i = 0; i < _countof(s_shaderTypes); i++)
    {
        const wchar_t* shaderTypes = s_shaderTypes[i];

        for (size_t j = 0; j < SHADER_TYPE_MAX; j++)
        {
            // opaq/punch have closesthit and anyhit, transparent hit groups only have anyhit
            for (size_t k = 0; k < (i == 1 ? 2 : 4); k++)
            {
                wchar_t exportName[0x100]{};
                wcscat_s(exportName, s_shaderHitGroups[j * s_hitGroupStride + k]);
                wcscat_s(exportName, shaderTypes);

                m_primaryStackSize = std::max(m_primaryStackSize, 
                    primaryStackSize + m_properties->GetShaderStackSize(exportName));
            }

            wchar_t exportName[0x100]{};
            wcscat_s(exportName, s_shaderHitGroups[j * s_hitGroupStride + 4]);
            wcscat_s(exportName, shaderTypes);

            m_secondaryStackSize = std::max(m_secondaryStackSize, 
                secondaryStackSize + m_properties->GetShaderStackSize(exportName));
        }
    }

    m_primaryStackSize = std::max(m_primaryStackSize, 
        primaryStackSize + m_properties->GetShaderStackSize(L"PrimaryMiss"));

    m_primaryStackSize = std::max(m_primaryStackSize, 
        primaryStackSize + m_properties->GetShaderStackSize(L"PrimaryTransparentMiss"));

    m_secondaryStackSize = std::max(m_secondaryStackSize, 
        secondaryStackSize + m_properties->GetShaderStackSize(L"SecondaryMiss"));

    m_secondaryStackSize = std::max(m_secondaryStackSize, 
        secondaryStackSize + m_properties->GetShaderStackSize(L"SecondaryEnvironmentColorMiss"));

    const wchar_t* rayGenShaderTable[] =
    {
        L"PrimaryRayGeneration",
        L"SecondaryRayGeneration"
    };

    static_assert(_countof(rayGenShaderTable) == RAY_GENERATION_NUM);

    m_rayGenShaderTable.resize(_countof(rayGenShaderTable) * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    for (size_t i = 0; i < _countof(rayGenShaderTable); i++)
        memcpy(&m_rayGenShaderTable[i * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT], m_properties->GetShaderIdentifier(rayGenShaderTable[i]), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    const wchar_t* missShaderTable[] =
    {
        L"PrimaryMiss",
        L"PrimaryTransparentMiss",
        L"SecondaryMiss",
        L"SecondaryEnvironmentColorMiss"
    };

    static_assert(_countof(missShaderTable) == MISS_NUM);

    m_missShaderTable.resize(_countof(missShaderTable) * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    for (size_t i = 0; i < _countof(missShaderTable); i++)
        memcpy(&m_missShaderTable[i * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES], m_properties->GetShaderIdentifier(missShaderTable[i]), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    for (size_t i = 0; i < _countof(m_hitGroups); i++)
        m_hitGroups[i] = m_properties->GetShaderIdentifier(s_shaderHitGroups[i]);

    for (auto& scratchBuffer : m_scratchBuffers)
    {
        createBuffer(
            D3D12_HEAP_TYPE_DEFAULT,
            SCRATCH_BUFFER_SIZE,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COMMON,
            scratchBuffer);

#ifdef _DEBUG
        scratchBuffer->GetResource()->SetName(L"Scratch Buffer");
#endif
    }

    m_uavId = m_descriptorHeap.allocateMany(s_textureCount);
    m_srvId = m_descriptorHeap.allocateMany(s_textureCount);

    m_colorRtvId = m_rtvDescriptorHeap.allocate();
    m_diffuseAlbedoRtvId = m_rtvDescriptorHeap.allocate();
    m_specularAlbedoRtvId = m_rtvDescriptorHeap.allocate();

    CD3DX12_ROOT_PARAMETER1 copyRootParams[2];
    copyRootParams[0].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    copyRootParams[1].InitAsDescriptorTable(_countof(srvDescriptorRanges), srvDescriptorRanges, D3D12_SHADER_VISIBILITY_PIXEL);

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
    copyPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
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

    m_qualityMode = static_cast<QualityMode>(
        iniFile.get<uint32_t>("Mod", "QualityMode", static_cast<uint32_t>(QualityMode::Auto)));
    
    const auto upscalerType = static_cast<UpscalerType>(
        iniFile.get<uint32_t>("Mod", "Upscaler", static_cast<uint32_t>(UpscalerType::FSR3)));
    
    if (upscalerType == UpscalerType::DLSS || upscalerType == UpscalerType::DLSSD)
    {
        auto ngx = std::make_unique<NGX>(*this);
        std::unique_ptr<DLSS> upscaler;
    
        if (ngx->valid())
        {
            if (upscalerType == UpscalerType::DLSSD && DLSSD::valid(*ngx))
                upscaler = std::make_unique<DLSSD>(*ngx);
            else if (upscalerType == UpscalerType::DLSS && DLSS::valid(*ngx))
                upscaler = std::make_unique<DLSS>(*ngx);
        }
    
        if (upscaler == nullptr)
        {
            MessageBox(nullptr,
                TEXT("An NVIDIA GPU with up to date drivers is required for DLSS. FSR3 will be used instead as a fallback."),
                TEXT("Generations Raytracing"),
                MB_ICONERROR);
    
            m_upscalerOverride[static_cast<size_t>(upscalerType)] = UpscalerType::FSR3;
        }
        else
        {
            m_ngx = std::move(ngx);
            m_upscaler = std::move(upscaler);
        }
    }
    
    if (m_upscaler == nullptr)
        m_upscaler = std::make_unique<FSR3>(*this);

    if (iniFile.getBool("Mod", "FrameGeneration", false))
        m_frameGenerator = std::make_unique<FrameGenerator>();

    D3D12_COMPUTE_PIPELINE_STATE_DESC resolvePipelineDesc{};
    resolvePipelineDesc.pRootSignature = m_raytracingRootSignature.Get();
    resolvePipelineDesc.CS.pShaderBytecode = ResolveComputeShader;
    resolvePipelineDesc.CS.BytecodeLength = sizeof(ResolveComputeShader);

    m_pipelineLibrary.createComputePipelineState(&resolvePipelineDesc, IID_PPV_ARGS(m_resolvePipeline.GetAddressOf()));

    D3D12_COMPUTE_PIPELINE_STATE_DESC resolveTransparencyPipelineDesc{};
    resolveTransparencyPipelineDesc.pRootSignature = m_raytracingRootSignature.Get();
    resolveTransparencyPipelineDesc.CS.pShaderBytecode = ResolveTransparencyComputeShader;
    resolveTransparencyPipelineDesc.CS.BytecodeLength = sizeof(ResolveTransparencyComputeShader);

    m_pipelineLibrary.createComputePipelineState(&resolveTransparencyPipelineDesc, IID_PPV_ARGS(m_resolveTransparencyPipeline.GetAddressOf()));

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

    CD3DX12_ROOT_PARAMETER1 im3dRootParams[2];
    im3dRootParams[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);
    im3dRootParams[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC, D3D12_SHADER_VISIBILITY_VERTEX);

    RootSignature::create(
        m_device.Get(),
        im3dRootParams,
        _countof(im3dRootParams),
        nullptr,
        0,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
        m_im3dRootSignature,
        "im3d");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC im3dPipelineDesc{};
    im3dPipelineDesc.pRootSignature = m_im3dRootSignature.Get();
    im3dPipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    im3dPipelineDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
    im3dPipelineDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    im3dPipelineDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    im3dPipelineDesc.SampleMask = ~0;
    im3dPipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    im3dPipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    im3dPipelineDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    im3dPipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    im3dPipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    im3dPipelineDesc.NumRenderTargets = 1;
    im3dPipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    im3dPipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    im3dPipelineDesc.SampleDesc.Count = 1;

    im3dPipelineDesc.VS.pShaderBytecode = Im3dTrianglesVertexShader;
    im3dPipelineDesc.VS.BytecodeLength = sizeof(Im3dTrianglesVertexShader);
    im3dPipelineDesc.PS.pShaderBytecode = Im3dTrianglesPixelShader;
    im3dPipelineDesc.PS.BytecodeLength = sizeof(Im3dTrianglesPixelShader);
    m_pipelineLibrary.createGraphicsPipelineState(&im3dPipelineDesc, IID_PPV_ARGS(m_im3dTrianglesPipeline.GetAddressOf()));

    im3dPipelineDesc.VS.pShaderBytecode = Im3dLinesVertexShader;
    im3dPipelineDesc.VS.BytecodeLength = sizeof(Im3dLinesVertexShader);
    im3dPipelineDesc.PS.pShaderBytecode = Im3dLinesPixelShader;
    im3dPipelineDesc.PS.BytecodeLength = sizeof(Im3dLinesPixelShader);
    m_pipelineLibrary.createGraphicsPipelineState(&im3dPipelineDesc, IID_PPV_ARGS(m_im3dLinesPipeline.GetAddressOf()));

    im3dPipelineDesc.VS.pShaderBytecode = Im3dPointsVertexShader;
    im3dPipelineDesc.VS.BytecodeLength = sizeof(Im3dPointsVertexShader);
    im3dPipelineDesc.PS.pShaderBytecode = Im3dPointsPixelShader;
    im3dPipelineDesc.PS.BytecodeLength = sizeof(Im3dPointsPixelShader);
    m_pipelineLibrary.createGraphicsPipelineState(&im3dPipelineDesc, IID_PPV_ARGS(m_im3dPointsPipeline.GetAddressOf()));

    D3D12_COMPUTE_PIPELINE_STATE_DESC reservoirPipelineDesc{};
    reservoirPipelineDesc.pRootSignature = m_raytracingRootSignature.Get();
    reservoirPipelineDesc.CS.pShaderBytecode = ReservoirComputeShader;
    reservoirPipelineDesc.CS.BytecodeLength = sizeof(ReservoirComputeShader);
    m_pipelineLibrary.createComputePipelineState(&reservoirPipelineDesc, IID_PPV_ARGS(m_reservoirPipeline.GetAddressOf()));

    CD3DX12_ROOT_PARAMETER1 grassInstancerRootParams[5];
    grassInstancerRootParams[0].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    grassInstancerRootParams[1].InitAsConstants(1, 0);
    grassInstancerRootParams[2].InitAsConstants(1, 1);
    grassInstancerRootParams[3].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);
    grassInstancerRootParams[4].InitAsUnorderedAccessView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);

    RootSignature::create(
        m_device.Get(),
        grassInstancerRootParams,
        _countof(grassInstancerRootParams),
        nullptr,
        0,
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS,
        m_grassInstancerRootSignature,
        "grass_instancer");

    D3D12_COMPUTE_PIPELINE_STATE_DESC grassInstancerPipelineDesc{};
    grassInstancerPipelineDesc.pRootSignature = m_grassInstancerRootSignature.Get();
    grassInstancerPipelineDesc.CS.pShaderBytecode = GrassInstancerComputeShader;
    grassInstancerPipelineDesc.CS.BytecodeLength = sizeof(GrassInstancerComputeShader);

    m_pipelineLibrary.createComputePipelineState(&grassInstancerPipelineDesc, IID_PPV_ARGS(m_grassInstancerPipeline.GetAddressOf()));
}

RaytracingDevice::~RaytracingDevice()
{
    if (m_device != nullptr)
    {
        ++m_fenceValue;

        m_graphicsQueue.signal(m_fenceValue);
        m_computeQueue.signal(m_fenceValue);
        m_copyQueue.signal(m_fenceValue);

        ID3D12Fence* fences[] =
        {
            m_graphicsQueue.getFence(),
            m_computeQueue.getFence(),
            m_copyQueue.getFence()
        };

        uint64_t fenceValues[] =
        {
            m_fenceValue,
            m_fenceValue,
            m_fenceValue
        };

        static_assert(_countof(fenceValues) == _countof(fences));

        m_device->SetEventOnMultipleFenceCompletion(
            fences,
            fenceValues,
            _countof(fences),
            D3D12_MULTIPLE_FENCE_WAIT_FLAG_ALL,
            nullptr);
    }
}
