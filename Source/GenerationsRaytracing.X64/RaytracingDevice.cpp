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
        triangles.IndexBuffer = m_indexBuffers[srcGeometryDesc.indexBufferId].allocation->GetResource()->GetGPUVirtualAddress() + srcGeometryDesc.indexOffset;
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

    auto& scratchBuffer = m_tempBuffers.emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        static_cast<uint32_t>(preBuildInfo.ScratchDataSizeInBytes),
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        scratchBuffer);

    accelStructDesc.DestAccelerationStructureData = bottomLevelAccelStruct->GetResource()->GetGPUVirtualAddress();
    accelStructDesc.ScratchAccelerationStructureData = scratchBuffer->GetResource()->GetGPUVirtualAddress();

    m_graphicsCommandList.getUnderlyingCommandList()->BuildRaytracingAccelerationStructure(&accelStructDesc, 0, nullptr);
    m_graphicsCommandList.uavBarrier(bottomLevelAccelStruct->GetResource());
}

void RaytracingDevice::procMsgReleaseRaytracingResource()
{
    const auto& message = m_messageReceiver.getMessage<MsgReleaseRaytracingResource>();

    switch (message.resourceType)
    {
    case MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct:
        m_tempBuffers.emplace_back(std::move(m_bottomLevelAccelStructs[message.resourceId]));
        break;

    default:
        assert(false);
        break;
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

    default:
        return false;
    }

    return true;
}
