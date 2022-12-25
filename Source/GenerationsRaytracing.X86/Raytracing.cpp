#include "Raytracing.h"

#include "Buffer.h"
#include "Message.h"
#include "MessageSender.h"

static CriticalSection criticalSection;
static std::unordered_set<const void*> modelSet;
static std::unordered_set<const hh::mr::CTerrainInstanceInfoData*> instanceSet;
static std::unordered_map<const hh::mr::CMeshData*, std::pair<ComPtr<Buffer>, size_t>> indexMap;

template<typename T>
static void createGeometry(const T& model, const hh::mr::CMeshData& mesh, bool opaque)
{
    std::lock_guard lock(criticalSection);

    auto& map = indexMap[&mesh];
    if (!map.first || !map.second)
        return;

    const auto msg = msgSender.start<MsgCreateGeometry>();

    msg->bottomLevelAS = (unsigned int)&model;
    msg->opaque = opaque;
    msg->vertexBuffer = (unsigned int)mesh.m_pD3DVertexBuffer;
    msg->vertexOffset = mesh.m_VertexOffset;
    msg->vertexCount = mesh.m_VertexNum;
    msg->vertexStride = mesh.m_VertexSize;
    msg->indexBuffer = (unsigned int)map.first.Get();
    msg->indexOffset = 0;
    msg->indexCount = map.second;

    msgSender.finish();
}

template<typename T>
static void createBottomLevelAS(const T& model)
{
    for (const auto& meshGroup : model.m_NodeGroupModels)
    {
        for (const auto& mesh : meshGroup->m_OpaqueMeshes) 
            createGeometry(model, *mesh, true);

        for (const auto& mesh : meshGroup->m_TransparentMeshes) 
            createGeometry(model, *mesh, false);

        for (const auto& mesh : meshGroup->m_PunchThroughMeshes) 
            createGeometry(model, *mesh, false);
    }

    for (const auto& mesh : model.m_OpaqueMeshes) 
        createGeometry(model, *mesh, true);

    for (const auto& mesh : model.m_TransparentMeshes) 
        createGeometry(model, *mesh, false);

    for (const auto& mesh : model.m_PunchThroughMeshes) 
        createGeometry(model, *mesh, false);

    const auto msg = msgSender.start<MsgCreateBottomLevelAS>();

    msg->bottomLevelAS = (unsigned int)&model;

    msgSender.finish();
}

static void traverse(const hh::mr::CRenderable* renderable)
{
    if (auto singleElement = dynamic_cast<const hh::mr::CSingleElement*>(renderable))
    {
        {
            std::lock_guard lock(criticalSection);

            if (modelSet.find(singleElement->m_spModel.get()) == modelSet.end())
            {
                createBottomLevelAS(*singleElement->m_spModel);
                modelSet.insert(singleElement->m_spModel.get());
            }
        }

        const auto msg = msgSender.start<MsgCreateInstance>();

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 4; j++)
                msg->transform[i * 4 + j] = singleElement->m_spInstanceInfo->m_Transform(j, i);
        }

        msg->bottomLevelAS = (unsigned int)singleElement->m_spModel.get();

        msgSender.finish();
    }
    else if (auto bundle = dynamic_cast<const hh::mr::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
            traverse(it.get());
    }
    else if (auto optimalBundle = dynamic_cast<const hh::mr::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
            traverse(it);
    }
}

static void SceneTraversed_Raytracing(uintptr_t, uintptr_t)
{
    auto document = Sonic::CGameDocument::GetInstance();
    if (!document)
        return;

    auto world = document->GetWorld();
    if (!world)
        return;

    auto& renderScene = world->m_pMember->m_spRenderScene;

    const hh::base::CStringSymbol symbols[] = {"Object", "Player"};

    for (const auto symbol : symbols)
    {
        const auto pair = renderScene->m_BundleMap.find(symbol);
        if (pair == renderScene->m_BundleMap.end())
            continue;

        traverse(pair->second.get());
    }

    for (const auto instance : instanceSet)
    {
        const auto msg = msgSender.start<MsgCreateInstance>();

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 4; j++)
                msg->transform[i * 4 + j] = (*instance->m_scpTransform)(i, j);
        }

        msg->bottomLevelAS = (unsigned int)instance->m_spTerrainModel.get();

        msgSender.finish();
    }

    msgSender.oneShot<MsgNotifySceneTraversed>();
}

static hh::fx::SScreenRenderParam SCREEN_RENDER_PARAM =
{
    nullptr,
    (void*)&SceneTraversed_Raytracing,
    0,
    {}
};

static const hh::fx::SDrawInstanceParam DRAW_INSTANCE_PARAM =
{
    0,
    0,
    nullptr,
    0xE1,
    (void*)0x651820,
    &SCREEN_RENDER_PARAM,
    0,
    {},
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    {
        {
            0xFF,
            0xFF,
            0xFF,
            0xFF
        }
    },
    3,
    0,
    0x101,
    0,
    0,
    0
};

HOOK(void, __fastcall, DestructModel, 0x4FA520, hh::mr::CModelData* This)
{
    {
        const auto msg = msgSender.start<MsgReleaseResource>();
        msg->resource = (unsigned int)This;
        msgSender.finish();

        std::lock_guard lock(criticalSection);
        modelSet.erase(This);
    }

    originalDestructModel(This);
}

HOOK(void, __fastcall, DestructTerrainModel, 0x717230, hh::mr::CTerrainModelData* This)
{
    {
        const auto msg = msgSender.start<MsgReleaseResource>();
        msg->resource = (unsigned int)This;
        msgSender.finish();

        std::lock_guard lock(criticalSection);
        modelSet.erase(This);
    }

    originalDestructTerrainModel(This);
}

HOOK(void, __cdecl, MakeMesh, 0x744CC0,
    hh::mr::CMeshData& mesh, 
    hl::hh::mirage::raw_mesh* data, 
    const hh::mr::CMirageDatabaseWrapper& databaseWrapper, 
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMesh(mesh, data, databaseWrapper, renderingInfrastructure);

    if (mesh.m_VertexNum == 0 || mesh.m_IndexNum <= 2)
        return;

    std::vector<hl::u16> indices;

    hl::u32 index = 0;
    hl::u16 a = HL_SWAP_U16(data->faces.data()[index++]);
    hl::u16 b = HL_SWAP_U16(data->faces.data()[index++]);
    int direction = -1;

    while (index < HL_SWAP_U32(data->faces.count))
    {
        hl::u16 c = HL_SWAP_U16(data->faces.data()[index++]);

        if (c == 0xFFFF)
        {
            a = HL_SWAP_U16(data->faces.data()[index++]);
            b = HL_SWAP_U16(data->faces.data()[index++]);
            direction = -1;
        }
        else
        {
            direction *= -1;

            if (a != b && b != c && c != a)
            {
                indices.push_back(a);

                if (direction > 0)
                {
                    indices.push_back(b);
                    indices.push_back(c);
                }
                else
                {
                    indices.push_back(c);
                    indices.push_back(b);
                }
            }

            a = b;
            b = c;
        }
    }

    assert(!indices.empty());

    const size_t dataSize = indices.size() * sizeof(hl::u16);

    ComPtr<Buffer> buffer;
    buffer.Attach(new Buffer(dataSize));

    const auto idxBufMsg = msgSender.start<MsgCreateIndexBuffer>();

    idxBufMsg->length = dataSize;
    idxBufMsg->format = D3DFMT_INDEX16;
    idxBufMsg->indexBuffer = (unsigned int)buffer.Get();

    msgSender.finish();

    const auto copyBufMsg = msgSender.start<MsgWriteBuffer>(dataSize);

    copyBufMsg->buffer = (unsigned int)buffer.Get();
    copyBufMsg->size = dataSize;

    memcpy(MSG_DATA_PTR(copyBufMsg), indices.data(), dataSize);

    msgSender.finish();

    indexMap[&mesh] = std::make_pair(std::move(buffer), indices.size());
}

HOOK(void, __fastcall, DestructMesh, 0x7227A0, hh::mr::CMeshData* This)
{
    {
        std::lock_guard lock(criticalSection);
        indexMap.erase(This);
    }

    originalDestructMesh(This);
}

HOOK(void, __cdecl, MakeTerrainInstanceInfo, 0x734D90, 
    const hh::base::CSharedString& name, 
    void* data,
    size_t dataSize,
    const boost::shared_ptr<hh::db::CDatabase>& database,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeTerrainInstanceInfo(name, data, dataSize, database, renderingInfrastructure);

    const auto instance = hh::mr::CMirageDatabaseWrapper(database.get())
        .GetTerrainInstanceInfoData(name);

    if (instance && 
        instance->m_spTerrainModel)
    {
        std::lock_guard lock(criticalSection);

        if (modelSet.find(instance->m_spTerrainModel.get()) == modelSet.end())
        {
            createBottomLevelAS(*instance->m_spTerrainModel);
            modelSet.insert(instance->m_spTerrainModel.get());
        }

        instanceSet.insert(instance.get());
    }
}

HOOK(void, __fastcall, DestructTerrainInstanceInfo, 0x717090, hh::mr::CTerrainInstanceInfoData* This)
{
    {
        std::lock_guard lock(criticalSection);
        instanceSet.erase(This);
    }

    originalDestructTerrainInstanceInfo(This);
}

void Raytracing::init()
{
    WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2
    WRITE_MEMORY(0x13DDC9C, void*, &DRAW_INSTANCE_PARAM); // Override game scene children
    WRITE_MEMORY(0x13DDCA0, size_t, 1); // Override game scene child count
    WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
    INSTALL_HOOK(DestructModel); // Garbage collection
    INSTALL_HOOK(DestructTerrainModel); // Garbage collection
    INSTALL_HOOK(MakeMesh); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(DestructMesh); // Garbage collection
    INSTALL_HOOK(MakeTerrainInstanceInfo); // Capture instances to put to TLAS
    INSTALL_HOOK(DestructTerrainInstanceInfo); // Garbage collection
    WRITE_MEMORY(0x722760, uint8_t, 0xC3); // Disable shared buffer
}