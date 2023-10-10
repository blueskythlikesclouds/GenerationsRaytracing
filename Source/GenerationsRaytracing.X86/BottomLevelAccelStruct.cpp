#include "BottomLevelAccelStruct.h"

#include "FreeListAllocator.h"
#include "GeometryFlags.h"
#include "IndexBuffer.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "TopLevelAccelStruct.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"

class MeshDataEx : public Hedgehog::Mirage::CMeshData
{
public:
    DX_PATCH::IDirect3DIndexBuffer9* m_indices;
    uint32_t m_indexCount;
};

HOOK(MeshDataEx*, __fastcall, MeshDataConstructor, 0x722860, MeshDataEx* This)
{
    const auto result = originalMeshDataConstructor(This);

    This->m_indices = nullptr;
    This->m_indexCount = 0;

    return result;
}

HOOK(void, __fastcall, MeshDataDestructor, 0x7227A0, MeshDataEx* This)
{
    if (This->m_indices != nullptr)
        This->m_indices->Release();

    originalMeshDataDestructor(This);
}

struct MeshResource
{
    const char* materialName;
    uint32_t indexCount;
    uint16_t* indices;
    // ...don't need the rest
};

static void convertToTriangles(MeshDataEx& meshData, const MeshResource* data)
{
    assert(meshData.m_indices == nullptr);

    const uint32_t indexNum = _byteswap_ulong(data->indexCount);
    if (indexNum <= 2)
        return;

    std::vector<uint16_t> indices;
    indices.reserve((indexNum - 2) * 3);

    uint16_t a = _byteswap_ushort(data->indices[0]);
    uint16_t b = _byteswap_ushort(data->indices[1]);
    bool direction = false;

    for (uint32_t i = 2; i < indexNum; i++)
    {
        uint16_t c = _byteswap_ushort(data->indices[i]);

        if (c == 0xFFFF)
        {
            a = _byteswap_ushort(data->indices[++i]);
            b = _byteswap_ushort(data->indices[++i]);
            direction = false;
        }
        else
        {
            direction = !direction;
            if (a != b && b != c && c != a)
            {
                if (direction)
                {
                    indices.push_back(c);
                    indices.push_back(b);
                    indices.push_back(a);
                }
                else
                {
                    indices.push_back(a);
                    indices.push_back(b);
                    indices.push_back(c);
                }
            }

            a = b;
            b = c;
        }
    }

    if (indices.empty())
        return;

    const size_t byteSize = indices.size() * sizeof(uint16_t);
    const auto indexBuffer = new IndexBuffer(byteSize);

    auto& createMessage = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
    createMessage.length = byteSize;
    createMessage.format = D3DFMT_INDEX16;
    createMessage.indexBufferId = indexBuffer->getId();
    s_messageSender.endMessage();

    auto& copyMessage = s_messageSender.makeMessage<MsgWriteIndexBuffer>(byteSize);
    copyMessage.indexBufferId = indexBuffer->getId();
    copyMessage.offset = 0;
    memcpy(copyMessage.data, indices.data(), byteSize);
    s_messageSender.endMessage();

    meshData.m_indices = reinterpret_cast<DX_PATCH::IDirect3DIndexBuffer9*>(indexBuffer);
    meshData.m_indexCount = indices.size();
}

HOOK(void, __cdecl, MakeMeshData, 0x744A00,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    if (!meshData.IsMadeOne())
        convertToTriangles(meshData, data);
    originalMakeMeshData(meshData, data, databaseWrapper, renderingInfrastructure);
}

HOOK(void, __cdecl, MakeMeshData2, 0x744CC0,
    MeshDataEx& meshData,
    const MeshResource* data,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    if (!meshData.IsMadeOne())
        convertToTriangles(meshData, data);
    originalMakeMeshData2(meshData, data, databaseWrapper, renderingInfrastructure);
}

template<typename T>
static void traverseMeshGroup(const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup, uint32_t flags, const T& function)
{
    for (const auto& meshData : meshGroup)
    {
        const auto& meshDataEx = *reinterpret_cast<const MeshDataEx*>(meshData.get());

        if (meshDataEx.m_indexCount > 2 && meshDataEx.m_VertexNum > 2 &&
            meshDataEx.m_indices != nullptr && meshDataEx.m_pD3DVertexBuffer != nullptr)
        {
            function(meshDataEx, flags);
        }
    }
}

template<typename TModelData, typename TFunction>
static void traverseModelData(const TModelData& modelData, const TFunction& function)
{
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        traverseMeshGroup(nodeGroupModelData->m_OpaqueMeshes, NULL, function);
        traverseMeshGroup(nodeGroupModelData->m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
        traverseMeshGroup(nodeGroupModelData->m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            traverseMeshGroup(specialMeshGroup, GEOMETRY_FLAG_TRANSPARENT, function);
    }

    traverseMeshGroup(modelData.m_OpaqueMeshes, NULL, function);
    traverseMeshGroup(modelData.m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
    traverseMeshGroup(modelData.m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);
}

template<typename T>
static void createBottomLevelAccelStruct(const T& modelData, uint32_t bottomLevelAccelStructId, uint32_t poseVertexBufferId)
{
    size_t geometryCount = 0;

    traverseModelData(modelData, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

    if (geometryCount == 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
        geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    message.bottomLevelAccelStructId = bottomLevelAccelStructId;
    message.preferFastBuild = poseVertexBufferId != NULL;
    memset(message.data, 0, geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data);
    uint32_t poseVertexOffset = 0;

    traverseModelData(modelData, [&](const MeshDataEx& meshDataEx, uint32_t flags)
    {
        if (poseVertexBufferId != NULL)
            flags |= GEOMETRY_FLAG_POSE;

        geometryDesc->flags = flags;
        geometryDesc->indexCount = meshDataEx.m_indexCount;
        geometryDesc->vertexCount = meshDataEx.m_VertexNum;
        geometryDesc->indexBufferId = reinterpret_cast<const IndexBuffer*>(meshDataEx.m_indices)->getId();

        uint32_t vertexOffset = meshDataEx.m_VertexOffset;
        if (poseVertexBufferId != NULL)
        {
            geometryDesc->vertexBufferId = poseVertexBufferId;
            vertexOffset = poseVertexOffset;
            poseVertexOffset += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position
        }
        else
        {
            geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
        }
        geometryDesc->vertexStride = meshDataEx.m_VertexSize;

        const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
            meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

        auto vertexElement = vertexDeclaration->getVertexElements();
        uint32_t texCoordOffsets[4]{};

        while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
        {
            const uint32_t offset = vertexOffset + vertexElement->Offset;

            switch (vertexElement->Usage)
            {
            case D3DDECLUSAGE_POSITION:
                geometryDesc->positionOffset = offset;
                break;

            case D3DDECLUSAGE_NORMAL:
                geometryDesc->normalOffset = offset;
                break;

            case D3DDECLUSAGE_TANGENT:
                geometryDesc->tangentOffset = offset;
                break;

            case D3DDECLUSAGE_BINORMAL:
                geometryDesc->binormalOffset = offset;
                break;

            case D3DDECLUSAGE_TEXCOORD:
                assert(vertexElement->UsageIndex < 4);
                texCoordOffsets[vertexElement->UsageIndex] = offset;
                break;

            case D3DDECLUSAGE_COLOR:
                geometryDesc->colorOffset = offset;
                if (vertexElement->Type != D3DDECLTYPE_FLOAT4)
                    geometryDesc->flags |= GEOMETRY_FLAG_D3DCOLOR;
                break;
            }

            ++vertexElement;
        }

        for (size_t i = 0; i < 4; i++)
            geometryDesc->texCoordOffsets[i] = texCoordOffsets[i] != 0 ? texCoordOffsets[i] : texCoordOffsets[0];

        const auto materialDataEx = reinterpret_cast<MaterialDataEx*>(
            meshDataEx.m_spMaterial.get());

        if (materialDataEx->m_materialId == NULL)
            materialDataEx->m_materialId = MaterialData::allocate();

        geometryDesc->materialId = materialDataEx->m_materialId;

        ++geometryDesc;
    });

    assert(reinterpret_cast<uint8_t*>(geometryDesc - geometryCount) == message.data);

    s_messageSender.endMessage();
}

static FreeListAllocator s_freeListAllocator;

HOOK(TerrainModelDataEx*, __fastcall, TerrainModelDataConstructor, 0x717440, TerrainModelDataEx* This)
{
    const auto result = originalTerrainModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;

    return result;
}

HOOK(void, __fastcall, TerrainModelDataDestructor, 0x717230, TerrainModelDataEx* This)
{
    if (This->m_bottomLevelAccelStructId != NULL)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();

        message.resourceType = MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct;
        message.resourceId = This->m_bottomLevelAccelStructId;

        s_messageSender.endMessage();

        s_freeListAllocator.free(This->m_bottomLevelAccelStructId);
    }

    originalTerrainModelDataDestructor(This);
}

static std::vector<TerrainModelDataEx*> s_terrainModelList;

static void __fastcall terrainModelDataSetMadeOne(TerrainModelDataEx* This)
{
    if (*reinterpret_cast<uint32_t*>(0x1B244D4) != NULL) // Share vertex buffer
    {
        s_terrainModelList.push_back(This);
    }
    else
    {
        if (This->m_bottomLevelAccelStructId == NULL)
            This->m_bottomLevelAccelStructId = s_freeListAllocator.allocate();

        createBottomLevelAccelStruct(*This, This->m_bottomLevelAccelStructId, NULL);
    }

    This->SetMadeOne();
}

HOOK(void, __fastcall, CreateShareVertexBuffer, 0x72EBD0, void* This)
{
    originalCreateShareVertexBuffer(This);

    for (const auto terrainModelData : s_terrainModelList)
    {
        if (terrainModelData->m_bottomLevelAccelStructId == NULL)
            terrainModelData->m_bottomLevelAccelStructId = s_freeListAllocator.allocate();

        createBottomLevelAccelStruct(*terrainModelData, terrainModelData->m_bottomLevelAccelStructId, NULL);
    }

    s_terrainModelList.clear();
}

HOOK(ModelDataEx*, __fastcall, ModelDataConstructor, 0x4FA400, ModelDataEx* This)
{
    const auto result = originalModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;

    return result;
}

HOOK(void, __fastcall, ModelDataDestructor, 0x4FA520, ModelDataEx* This)
{
    if (This->m_bottomLevelAccelStructId != NULL)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
        message.resourceType = MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct;
        message.resourceId = This->m_bottomLevelAccelStructId;
        s_messageSender.endMessage();

        s_freeListAllocator.free(This->m_bottomLevelAccelStructId);
    }

    originalModelDataDestructor(This);
}

uint32_t BottomLevelAccelStruct::allocate()
{
    return s_freeListAllocator.allocate();
}

void BottomLevelAccelStruct::create(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap, bool isEnabled)
{
    if (!isEnabled)
    {
        if (instanceInfoEx.m_instanceId != NULL)
        {
            auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
            message.resourceType = MsgReleaseRaytracingResource::ResourceType::Instance;
            message.resourceId = instanceInfoEx.m_instanceId;
            s_messageSender.endMessage();

            TopLevelAccelStruct::free(instanceInfoEx.m_instanceId);
            instanceInfoEx.m_instanceId = NULL;
        }

        return;
    }

    for (auto& [key, value] : materialMap)
    {
        MaterialData::create(*value);
    }

    auto transform = instanceInfoEx.m_Transform;
    uint32_t bottomLevelAccelStructId;

    if (instanceInfoEx.m_spPose != nullptr && instanceInfoEx.m_spPose->GetMatrixNum() > 1)
    {
        if (instanceInfoEx.m_poseVertexBuffer == nullptr)
        {
            instanceInfoEx.m_poseVertexBuffer.Attach(new VertexBuffer(NULL));

            auto& message = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
            message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
            message.allowUnorderedAccess = true;

            uint32_t length = 0;
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                auto& copyMessage = s_messageSender.makeMessage<MsgCopyVertexBuffer>();
                copyMessage.dstVertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
                copyMessage.dstOffset = length;
                copyMessage.srcVertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
                copyMessage.srcOffset = meshDataEx.m_VertexOffset;
                copyMessage.numBytes = meshDataEx.m_VertexNum * meshDataEx.m_VertexSize;
                s_messageSender.endMessage();

                length += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position

            });

            message.length = length;
            s_messageSender.endMessage();
        }

        uint32_t geometryCount = 0;
        traverseModelData(modelDataEx, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

        auto& message = s_messageSender.makeMessage<MsgComputePose>(
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix) +
            geometryCount * sizeof(MsgComputePose::GeometryDesc));

        message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
        message.nodeCount = static_cast<uint8_t>(instanceInfoEx.m_spPose->GetMatrixNum());
        message.geometryCount = geometryCount;

        memcpy(message.data, instanceInfoEx.m_spPose->GetMatrixList(),
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        auto geometryDesc = reinterpret_cast<MsgComputePose::GeometryDesc*>(message.data +
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        memset(geometryDesc, 0, geometryCount * sizeof(MsgComputePose::GeometryDesc));

        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            assert(meshDataEx.m_VertexOffset == 0);

            geometryDesc->vertexCount = meshDataEx.m_VertexNum;
            geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
            geometryDesc->vertexStride = static_cast<uint8_t>(meshDataEx.m_VertexSize);

            if (meshDataEx.m_pNodeIndices != nullptr)
                memcpy(geometryDesc->nodePalette, meshDataEx.m_pNodeIndices, std::min(25u, meshDataEx.m_NodeNum));

            const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
                meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

            auto vertexElement = vertexDeclaration->getVertexElements();

            while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
            {
                const uint8_t offset = static_cast<uint8_t>(vertexElement->Offset);

                switch (vertexElement->Usage)
                {
                case D3DDECLUSAGE_NORMAL:
                    geometryDesc->normalOffset = offset;
                    break;

                case D3DDECLUSAGE_TANGENT:
                    geometryDesc->tangentOffset = offset;
                    break;

                case D3DDECLUSAGE_BINORMAL:
                    geometryDesc->binormalOffset = offset;
                    break;

                case D3DDECLUSAGE_BLENDWEIGHT:
                    geometryDesc->blendWeightOffset = offset;
                    break;

                case D3DDECLUSAGE_BLENDINDICES:
                    geometryDesc->blendIndicesOffset = offset;
                    break;
                }

                ++vertexElement;
            }

            ++geometryDesc;
        });

        s_messageSender.endMessage();

        if (instanceInfoEx.m_bottomLevelAccelStructId == NULL)
        {
            instanceInfoEx.m_bottomLevelAccelStructId = s_freeListAllocator.allocate();
            createBottomLevelAccelStruct(modelDataEx,
                instanceInfoEx.m_bottomLevelAccelStructId, instanceInfoEx.m_poseVertexBuffer->getId());
        }
        else
        {
            auto& buildMessage = s_messageSender.makeMessage<MsgBuildBottomLevelAccelStruct>();
            buildMessage.bottomLevelAccelStructId = instanceInfoEx.m_bottomLevelAccelStructId;
            s_messageSender.endMessage();
        }
        bottomLevelAccelStructId = instanceInfoEx.m_bottomLevelAccelStructId;
    }
    else
    {
        if (instanceInfoEx.m_spPose != nullptr)
            transform = transform * (*instanceInfoEx.m_spPose->GetMatrixList());

        if (modelDataEx.m_bottomLevelAccelStructId == NULL)
        {
            modelDataEx.m_bottomLevelAccelStructId = s_freeListAllocator.allocate();
            createBottomLevelAccelStruct(modelDataEx, modelDataEx.m_bottomLevelAccelStructId, NULL);
        }
        bottomLevelAccelStructId = modelDataEx.m_bottomLevelAccelStructId;
    }

    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(
        materialMap.size() * sizeof(uint32_t) * 2);

    for (size_t i = 0; i < 3; i++)
    {
        for (size_t j = 0; j < 4; j++)
            message.transform[i][j] = transform(i, j);
    }

    if (instanceInfoEx.m_instanceId == NULL)
    {
        instanceInfoEx.m_instanceId = TopLevelAccelStruct::allocate();
        message.storePrevTransform = false;
    }
    else
    {
        message.storePrevTransform = true;
    }

    message.instanceId = instanceInfoEx.m_instanceId;
    message.bottomLevelAccelStructId = bottomLevelAccelStructId;

    auto materialIds = reinterpret_cast<uint32_t*>(message.data);

    for (auto& [key, value] : materialMap)
    {
        *materialIds = reinterpret_cast<MaterialDataEx*>(key)->m_materialId;
        ++materialIds;

        *materialIds = reinterpret_cast<MaterialDataEx*>(value.get())->m_materialId;
        ++materialIds;
    }

    s_messageSender.endMessage();
}

void BottomLevelAccelStruct::release(uint32_t bottomLevelAccelStructId)
{
    auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
    message.resourceType = MsgReleaseRaytracingResource::ResourceType::BottomLevelAccelStruct;
    message.resourceId = bottomLevelAccelStructId;
    s_messageSender.endMessage();

    s_freeListAllocator.free(bottomLevelAccelStructId);
}

void BottomLevelAccelStruct::init()
{
    WRITE_MEMORY(0x72294D, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x739511, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x739641, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x7397D1, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C763, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C873, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73C9EA, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D063, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D173, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D2EA, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73D971, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73DA86, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73DBFE, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E383, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E493, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73E606, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73EF23, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73F033, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x73F1A6, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745661, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745771, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x7458E4, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0x745D01, uint32_t, sizeof(MeshDataEx));
    WRITE_MEMORY(0xCD9A99, uint32_t, sizeof(MeshDataEx));

    INSTALL_HOOK(MeshDataConstructor);
    INSTALL_HOOK(MeshDataDestructor);

    INSTALL_HOOK(MakeMeshData);
    INSTALL_HOOK(MakeMeshData2);

    WRITE_MEMORY(0x72FCDC, uint8_t, sizeof(TerrainModelDataEx));

    INSTALL_HOOK(TerrainModelDataConstructor);
    INSTALL_HOOK(TerrainModelDataDestructor);

    WRITE_CALL(0x73A6D3, terrainModelDataSetMadeOne);
    WRITE_CALL(0x739AB4, terrainModelDataSetMadeOne);

    INSTALL_HOOK(CreateShareVertexBuffer);

    WRITE_MEMORY(0x4FA1FC, uint32_t, sizeof(ModelDataEx));
    WRITE_MEMORY(0xE993F1, uint32_t, sizeof(ModelDataEx));

    INSTALL_HOOK(ModelDataConstructor);
    INSTALL_HOOK(ModelDataDestructor);
}
