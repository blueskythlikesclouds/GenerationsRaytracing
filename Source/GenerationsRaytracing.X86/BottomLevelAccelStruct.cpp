#include "BottomLevelAccelStruct.h"

#include "FreeListAllocator.h"
#include "IndexBuffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "VertexBuffer.h"

static FreeListAllocator s_freeListAllocator;

class TerrainModelDataEx : public Hedgehog::Mirage::CTerrainModelData
{
public:
    uint32_t m_bottomLevelAccelStructId;
};

HOOK(TerrainModelDataEx*, __fastcall, TerrainModelDataConstructor, 0x717440, TerrainModelDataEx* This)
{
    const auto result = originalTerrainModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = s_freeListAllocator.allocate();

    return result;
}

HOOK(void, __fastcall, TerrainModelDataDestructor, 0x717230, TerrainModelDataEx* This)
{
    if (This->m_bottomLevelAccelStructId != NULL)
    {
        auto& message = s_messageSender.makeParallelMessage<MsgReleaseRaytracingResource>();

        message.resourceType = MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct;
        message.resourceId = This->m_bottomLevelAccelStructId;

        s_messageSender.endParallelMessage();

        s_freeListAllocator.free(This->m_bottomLevelAccelStructId);
    }

    originalTerrainModelDataDestructor(This);
}

static bool checkMeshValidity(const Hedgehog::Mirage::CMeshData& meshData)
{
    return meshData.m_IndexNum > 2 && meshData.m_VertexNum > 2 &&
        meshData.m_pD3DIndexBuffer != nullptr && meshData.m_pD3DVertexBuffer != nullptr;
}

static size_t getGeometryCount(const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup)
{
    size_t geometryCount = 0;
    for (const auto& meshData : meshGroup)
    {
        if (checkMeshValidity(*meshData))
            ++geometryCount;
    }
    return geometryCount;
}

template<typename T>
static size_t getGeometryCount(const T& modelData)
{
    size_t geometryCount = 0;

    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        geometryCount += getGeometryCount(nodeGroupModelData->m_OpaqueMeshes);
        geometryCount += getGeometryCount(nodeGroupModelData->m_TransparentMeshes);
        geometryCount += getGeometryCount(nodeGroupModelData->m_PunchThroughMeshes);

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            geometryCount += getGeometryCount(specialMeshGroup);
    }

    geometryCount += getGeometryCount(modelData.m_OpaqueMeshes);
    geometryCount += getGeometryCount(modelData.m_TransparentMeshes);
    geometryCount += getGeometryCount(modelData.m_PunchThroughMeshes);

    return geometryCount;
}

static MsgCreateBottomLevelAccelStruct::GeometryDesc* convertMeshGroupToGeometryDescs(
    const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup,
    MsgCreateBottomLevelAccelStruct::GeometryDesc* geometryDesc)
{
    for (const auto& mesh : meshGroup)
    {
        if (!checkMeshValidity(*mesh))
            continue;

        geometryDesc->indexCount = mesh->m_IndexNum;
        geometryDesc->vertexCount = mesh->m_VertexNum;
        geometryDesc->indexBufferId = reinterpret_cast<const IndexBuffer*>(mesh->m_pD3DIndexBuffer)->getId();
        geometryDesc->indexOffset = mesh->m_IndexOffset * 2;
        geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(mesh->m_pD3DVertexBuffer)->getId();
        geometryDesc->vertexOffset = mesh->m_VertexOffset;
        geometryDesc->vertexStride = mesh->m_VertexSize;

        ++geometryDesc;
    }
    return geometryDesc;
}

template<typename T>
static MsgCreateBottomLevelAccelStruct::GeometryDesc* convertModelToGeometryDescs(
    const T& modelData,
    MsgCreateBottomLevelAccelStruct::GeometryDesc* geometryDesc)
{
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_OpaqueMeshes, geometryDesc);
        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_TransparentMeshes, geometryDesc);
        geometryDesc = convertMeshGroupToGeometryDescs(nodeGroupModelData->m_PunchThroughMeshes, geometryDesc);

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            geometryDesc = convertMeshGroupToGeometryDescs(specialMeshGroup, geometryDesc);
    }

    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_OpaqueMeshes, geometryDesc);
    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_TransparentMeshes, geometryDesc);
    geometryDesc = convertMeshGroupToGeometryDescs(modelData.m_PunchThroughMeshes, geometryDesc);

    return geometryDesc;
}

static void __fastcall terrainModelDataSetMadeOne(TerrainModelDataEx* terrainModelData)
{
    assert(terrainModelData->m_bottomLevelAccelStructId != NULL);

    const size_t geometryCount = getGeometryCount(*terrainModelData);

    auto& message = s_messageSender.makeParallelMessage<MsgCreateBottomLevelAccelStruct>(
        geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    message.bottomLevelAccelStructId = terrainModelData->m_bottomLevelAccelStructId;

    const auto geometryDesc = convertModelToGeometryDescs(*terrainModelData, 
        reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data));

    assert(reinterpret_cast<uint8_t*>(geometryDesc - geometryCount) == message.data);

    s_messageSender.endParallelMessage();

    terrainModelData->SetMadeOne();
}

void BottomLevelAccelStruct::init()
{
    WRITE_MEMORY(0x72FCDC, uint8_t, sizeof(TerrainModelDataEx));

    INSTALL_HOOK(TerrainModelDataConstructor);
    INSTALL_HOOK(TerrainModelDataDestructor);

    WRITE_CALL(0x73A6D3, terrainModelDataSetMadeOne);
    WRITE_CALL(0x739AB4, terrainModelDataSetMadeOne);
}
