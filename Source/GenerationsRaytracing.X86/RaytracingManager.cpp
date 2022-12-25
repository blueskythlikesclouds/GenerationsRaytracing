#include "RaytracingManager.h"

#include "Buffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"
#include "VertexDeclaration.h"

static CriticalSection criticalSection;
static std::unordered_set<hh::mr::CMaterialData*> pendingMaterialSet;
static std::unordered_set<const hh::mr::CMaterialData*> materialSet;
static std::unordered_set<const void*> modelSet;
static std::unordered_set<hh::mr::CTerrainInstanceInfoData*> instanceSet;
static std::unordered_map<const hh::mr::CMeshData*, std::pair<ComPtr<Buffer>, size_t>> indexMap;

template<typename T>
static void createGeometry(const T& model, const hh::mr::CMeshData& mesh, bool opaque)
{
    std::lock_guard lock(criticalSection);
    auto& indexBuffer = indexMap[&mesh];

    const auto msg = msgSender.start<MsgCreateGeometry>();

    msg->bottomLevelAS = (unsigned int)&model;
    msg->opaque = opaque;
    msg->vertexBuffer = reinterpret_cast<Buffer*>(mesh.m_pD3DVertexBuffer)->id;
    msg->vertexOffset = mesh.m_VertexOffset;
    msg->vertexCount = mesh.m_VertexNum;
    msg->vertexStride = mesh.m_VertexSize;
    msg->indexBuffer = indexBuffer.first->id;
    msg->indexOffset = 0;
    msg->indexCount = indexBuffer.second;

    for (const auto& element : reinterpret_cast<VertexDeclaration*>(mesh.m_VertexDeclarationPtr.m_pD3DVertexDeclaration)->vertexElements)
    {
        if (element.UsageIndex > 0) continue;

        switch (element.Usage)
        {
        case D3DDECLUSAGE_NORMAL: 
            msg->normalOffset = element.Offset;
            break;

        case D3DDECLUSAGE_TANGENT: 
            msg->tangentOffset = element.Offset;
            break;

        case D3DDECLUSAGE_BINORMAL: 
            msg->binormalOffset = element.Offset;
            break;

        case D3DDECLUSAGE_TEXCOORD:
            msg->texCoordOffset = element.Offset;
            break;

        case D3DDECLUSAGE_COLOR: 
            msg->colorOffset = element.Offset;
            break;
        }
    }

    msg->material = (unsigned int)mesh.m_spMaterial.get();

    assert(mesh.m_pD3DVertexBuffer && indexBuffer.first);

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
        std::lock_guard lock(criticalSection);

        if (singleElement->m_spModel->IsMadeAll() && modelSet.find(singleElement->m_spModel.get()) != modelSet.end())
        {
            const auto msg = msgSender.start<MsgCreateInstance>();

            for (size_t i = 0; i < 3; i++)
                for (size_t j = 0; j < 4; j++)
                    msg->transform[i * 4 + j] = singleElement->m_spInstanceInfo->m_Transform(i, j);

            msg->bottomLevelAS = (unsigned int)singleElement->m_spModel.get();

            msgSender.finish();
        }
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

static FUNCTION_PTR(void, __cdecl, SceneRender, 0x652110, void* A1);

static void __cdecl SceneRender_Raytracing(void* A1)
{
    SceneRender(A1);

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

    std::lock_guard lock(criticalSection);

    for (auto it = pendingMaterialSet.begin(); it != pendingMaterialSet.end();)
    {
        if ((*it)->IsMadeAll())
        {
            const auto msg = msgSender.start<MsgCreateMaterial>();

            msg->material = (unsigned int)*it;
            memset(msg->textures, 0, sizeof(msg->textures));

            size_t index = 0;
            for (auto& texture : (*it)->m_spTexsetData->m_TextureList)
            {
                if (texture->m_spPictureData)
                    msg->textures[index++] = reinterpret_cast<Texture*>(texture->m_spPictureData->m_pD3DTexture)->id;
            }

            msgSender.finish();

            materialSet.insert(*it);
            it = pendingMaterialSet.erase(it);
        }
        else
            ++it;
    }

    for (const auto instance : instanceSet)
    {
        if (!instance->IsMadeAll() || modelSet.find(instance->m_spTerrainModel.get()) == modelSet.end())
            continue;

        const auto msg = msgSender.start<MsgCreateInstance>();

        for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 4; j++)
                msg->transform[i * 4 + j] = (*instance->m_scpTransform)(i, j);

        msg->bottomLevelAS = (unsigned int)instance->m_spTerrainModel.get();

        msgSender.finish();
    }

    msgSender.oneShot<MsgNotifySceneTraversed>();
}

HOOK(void, __cdecl, MakeMaterial, 0x733E60,
    const hh::base::CSharedString& name,
    void* data,
    size_t dataSize,
    const boost::shared_ptr<hh::db::CDatabase>& database,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMaterial(name, data, dataSize, database, renderingInfrastructure);

    const auto material = hh::mr::CMirageDatabaseWrapper(database.get())
        .GetMaterialData(name);

    if (!material->m_spTexsetData)
        return;

    std::lock_guard lock(criticalSection);
    pendingMaterialSet.insert(material.get());
}

static void convertToTriangles(const hh::mr::CMeshData& mesh, hl::hh::mirage::raw_mesh* data)
{
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
    idxBufMsg->indexBuffer = buffer->id;

    msgSender.finish();

    const auto copyBufMsg = msgSender.start<MsgWriteBuffer>(dataSize);

    copyBufMsg->buffer = buffer->id;
    copyBufMsg->size = dataSize;

    memcpy(MSG_DATA_PTR(copyBufMsg), indices.data(), dataSize);

    msgSender.finish();

    std::lock_guard lock(criticalSection);
    indexMap[&mesh] = std::make_pair(std::move(buffer), indices.size());
}

HOOK(void, __cdecl, MakeMesh, 0x744A00,
    hh::mr::CMeshData& mesh,
    hl::hh::mirage::raw_mesh* data,
    const hh::mr::CMirageDatabaseWrapper& databaseWrapper,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMesh(mesh, data, databaseWrapper, renderingInfrastructure);
    convertToTriangles(mesh, data);
}

HOOK(void, __cdecl, MakeMesh2, 0x744CC0,
    hh::mr::CMeshData& mesh,
    hl::hh::mirage::raw_mesh* data,
    const hh::mr::CMirageDatabaseWrapper& databaseWrapper,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeMesh2(mesh, data, databaseWrapper, renderingInfrastructure);
    convertToTriangles(mesh, data);
}

HOOK(void, __cdecl, MakeModel, 0x7337A0,
    const hh::base::CSharedString& name,
    void* data,
    size_t dataSize,
    const boost::shared_ptr<hh::db::CDatabase>& database,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeModel(name, data, dataSize, database, renderingInfrastructure);

    const auto model = hh::mr::CMirageDatabaseWrapper(database.get())
        .GetModelData(name);

    std::lock_guard lock(criticalSection);

    if ((model->m_Flags & hh::db::eDatabaseDataFlags_IsMadeOne) && modelSet.find(model.get()) == modelSet.end())
    {
        modelSet.insert(model.get());
        createBottomLevelAS(*model);
    }
}

HOOK(void, __cdecl, MakeTerrainModel, 0x734960,
    const hh::base::CSharedString& name,
    void* data,
    size_t dataSize,
    const boost::shared_ptr<hh::db::CDatabase>& database,
    hh::mr::CRenderingInfrastructure& renderingInfrastructure)
{
    originalMakeTerrainModel(name, data, dataSize, database, renderingInfrastructure);

    const auto terrainModel = hh::mr::CMirageDatabaseWrapper(database.get())
        .GetTerrainModelData(name);

    std::lock_guard lock(criticalSection);

    if ((terrainModel->m_Flags & hh::db::eDatabaseDataFlags_IsMadeOne) && modelSet.find(terrainModel.get()) == modelSet.end())
    {
        modelSet.insert(terrainModel.get());
        createBottomLevelAS(*terrainModel);
    }
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

    if (instance->m_Flags & hh::db::eDatabaseDataFlags_IsMadeOne)
    {
        std::lock_guard lock(criticalSection);
        instanceSet.insert(instance.get());
    }
}

HOOK(void, __fastcall, DestructMaterial, 0x704B80, hh::mr::CMaterialData* This)
{
    {
        std::lock_guard lock(criticalSection);
        pendingMaterialSet.erase(This);
        materialSet.erase(This);
    }

    originalDestructMaterial(This);
}

HOOK(void, __fastcall, DestructMesh, 0x7227A0, hh::mr::CMeshData* This)
{
    {
        std::lock_guard lock(criticalSection);
        indexMap.erase(This);
    }

    originalDestructMesh(This);
}

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

HOOK(void, __fastcall, DestructTerrainInstanceInfo, 0x717090, hh::mr::CTerrainInstanceInfoData* This)
{
    {
        std::lock_guard lock(criticalSection);
        instanceSet.erase(This);
    }

    originalDestructTerrainInstanceInfo(This);
}

void RaytracingManager::init()
{
    WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2

    WRITE_MEMORY(0x13DC2D8, void*, &SceneRender_Raytracing); // Override scene render function
    WRITE_MEMORY(0x13DDCA0, size_t, 1); // Override game scene child count

    WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
    WRITE_MEMORY(0x722760, uint8_t, 0xC3); // Disable shared buffer

    INSTALL_HOOK(MakeMaterial); // Capture materials to use in shader
    INSTALL_HOOK(MakeMesh); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeMesh2); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeModel); // Capture models to construct BLAS
    INSTALL_HOOK(MakeTerrainModel); // Capture terrain models to construct BLAS
    INSTALL_HOOK(MakeTerrainInstanceInfo); // Capture instances to put to TLAS

    INSTALL_HOOK(DestructMaterial); // Garbage collection
    INSTALL_HOOK(DestructMesh); // Garbage collection
    INSTALL_HOOK(DestructModel); // Garbage collection
    INSTALL_HOOK(DestructTerrainModel); // Garbage collection
    INSTALL_HOOK(DestructTerrainInstanceInfo); // Garbage collection

}