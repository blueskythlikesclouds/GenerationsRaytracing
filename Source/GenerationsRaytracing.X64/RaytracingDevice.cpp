#include "RaytracingDevice.h"

#include "DxgiConverter.h"
#include "Message.h"

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

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    m_graphicsCommandLists[m_frame].uavBarrier(bottomLevelAccelStruct->GetResource());
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
        m_instanceDescs[message.resourceId].AccelerationStructure = NULL;
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
        instanceDesc.AccelerationStructure = 
            m_bottomLevelAccelStructs[message.bottomLevelAccelStructId]->GetResource()->GetGPUVirtualAddress();
    }
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

    default:
        return false;
    }

    return true;
}

void RaytracingDevice::createTopLevelAccelStruct()
{
    if (m_instanceDescs.empty())
        return;

    for (const auto&[instanceId, bottomLevelAccelStructId] : m_deferredInstances)
    {
        if (m_bottomLevelAccelStructs.size() > bottomLevelAccelStructId && 
            m_bottomLevelAccelStructs[bottomLevelAccelStructId] != nullptr)
        {
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
        static_cast<uint32_t>(m_instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC)), 0x10);

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

    m_graphicsCommandLists[m_frame].getUnderlyingCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    m_graphicsCommandLists[m_frame].uavBarrier(topLevelAccelStruct->GetResource());
}

void RaytracingDevice::updateRaytracing()
{
    createTopLevelAccelStruct();
}
