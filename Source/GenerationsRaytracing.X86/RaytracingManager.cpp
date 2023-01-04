#include "RaytracingManager.h"

#include "Buffer.h"
#include "Identifier.h"
#include "Message.h"
#include "MessageSender.h"
#include "ShaderMapping.h"
#include "Texture.h"
#include "VertexDeclaration.h"

static tbb::task_group group;

static CriticalSection identifierMutex;
static CriticalSection instanceMutex;
static CriticalSection indexMutex;
static CriticalSection matrixMutex;

static ShaderMapping shaderMapping;

static std::unordered_map<hh::db::CDatabaseData*, size_t> identifierMap;

static std::unordered_set<hh::mr::CTerrainInstanceInfoData*> instanceSet;
static std::unordered_map<const hh::mr::CMeshData*, std::pair<ComPtr<Buffer>, size_t>> indexMap;

static std::unordered_map<const void*, hh::math::CMatrix> prevMatrixMap;

static bool isMadeForBridge(const hh::db::CDatabaseData& databaseData)
{
    return *(bool*)((uintptr_t)&databaseData + 5);
}

static void setMadeForBridge(const hh::db::CDatabaseData& databaseData, bool value = true)
{
    *(bool*)((uintptr_t)&databaseData + 5) = value;
}

struct SearchResult
{
    size_t id;
    bool shouldCreate;
};

static SearchResult searchDatabaseData(hh::db::CDatabaseData& databaseData)
{
    std::lock_guard lock(identifierMutex);

    SearchResult result{};
    const auto pair = identifierMap.find(&databaseData);

    if (isMadeForBridge(databaseData) && pair != identifierMap.end())
    {
        result.id = pair->second;
        result.shouldCreate = false;
    }
    else
    {
        result.id = getNextIdentifier();
        result.shouldCreate = true;

        setMadeForBridge(databaseData);
        identifierMap[&databaseData] = result.id;
    }

    return result;
}

static void createMaterial(hh::mr::CMaterialData& material, size_t id)
{
    group.run([&material, id]
    {
        const auto msg = msgSender.start<MsgCreateMaterial>();

        msg->material = id;

        memset(msg->shader, 0, sizeof(msg->shader));
        memset(msg->textures, 0, sizeof(msg->textures));
        memset(msg->parameters, 0, sizeof(msg->parameters));

        if (material.m_spShaderListData != nullptr)
        {
            const auto pair = shaderMapping.indices.find(material.m_spShaderListData->m_TypeAndName.c_str() + sizeof("Mirage.shader-list"));

            if (pair != shaderMapping.indices.end())
            {
                const auto& shader = shaderMapping.shaders[pair->second];
                strcpy(msg->shader, shader.name.c_str());

                std::unordered_map<hh::base::SSymbolNode*, size_t> counts;

                for (const auto& texture : material.m_spTexsetData->m_TextureList)
                {
                    const size_t target = counts[texture->m_Type.m_pSymbolNode];
                    size_t current = 0;

                    for (size_t i = 0; i < shader.textures.size(); i++)
                    {
                        if (shader.textures[i] == texture->m_Type.GetValue() && (current++) == target)
                        {
                            msg->textures[i] = texture->m_spPictureData && texture->m_spPictureData->m_pD3DTexture ?
                                reinterpret_cast<Texture*>(texture->m_spPictureData->m_pD3DTexture)->id : NULL;

                            ++counts[texture->m_Type.m_pSymbolNode];
                            break;
                        }
                    }
                }

                for (const auto& parameter : material.m_Float4Params)
                {
                    for (size_t i = 0; i < shader.parameters.size(); i++)
                    {
                        if (shader.parameters[i] == parameter->m_Name.GetValue())
                        {
                            memcpy(msg->parameters[i], parameter->m_spValue.get(), sizeof(msg->parameters[i]));
                            break;
                        }
                    }
                }
            }
        }

        msgSender.finish();
    });
}

static size_t createMaterial(hh::mr::CMaterialData& material)
{
    const auto result = searchDatabaseData(material);

    if (result.shouldCreate)
        createMaterial(material, result.id);

    return result.id;
}

static void createGeometry(const size_t blasId, const size_t instanceInfo, const hh::mr::CMeshData & mesh, bool opaque, bool punchThrough, bool isSkeletalModel)
{
    if (mesh.m_VertexNum == 0 || mesh.m_IndexNum <= 2)
        return;

    const size_t materialId = createMaterial(*mesh.m_spMaterial);

    indexMutex.lock();
    auto& indexBuffer = indexMap[&mesh];
    indexMutex.unlock();

    if (!indexBuffer.first)
        return;

    const size_t nodeNum = isSkeletalModel ? mesh.m_NodeNum : 0;

    const auto msg = msgSender.start<MsgCreateGeometry>(nodeNum);

    msg->bottomLevelAS = blasId;
    msg->instanceInfo = instanceInfo;
    msg->opaque = opaque;
    msg->punchThrough = punchThrough;
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
            msg->colorFormat = element.Type != D3DDECLTYPE_FLOAT4;
            break;

        case D3DDECLUSAGE_BLENDWEIGHT:
            msg->blendWeightOffset = element.Offset;
            break;

        case D3DDECLUSAGE_BLENDINDICES:
            msg->blendIndicesOffset = element.Offset;
            break;
        }
    }

    msg->material = materialId;

    assert(mesh.m_pD3DVertexBuffer && indexBuffer.first);

    msg->nodeNum = nodeNum;
    memcpy(MSG_DATA_PTR(msg), mesh.m_pNodeIndices, nodeNum);

    msgSender.finish();
}

template<typename T>
static void createBottomLevelAS(T& model, size_t blasId, size_t instanceInfo, bool isSkeletalModel)
{
    for (const auto& meshGroup : model.m_NodeGroupModels)
    {
        if (!meshGroup->m_Visible)
            continue;
    
        for (const auto& mesh : meshGroup->m_OpaqueMeshes) 
            createGeometry(blasId, instanceInfo, *mesh, true, false, isSkeletalModel);
    
        for (const auto& mesh : meshGroup->m_TransparentMeshes) 
            createGeometry(blasId, instanceInfo, *mesh, false, false, isSkeletalModel);
    
        for (const auto& mesh : meshGroup->m_PunchThroughMeshes) 
            createGeometry(blasId, instanceInfo, *mesh, false, true, isSkeletalModel);
    
        for (const auto& specialMeshGroup : meshGroup->m_SpecialMeshGroups)
        {
            for (const auto& mesh : specialMeshGroup)
                createGeometry(blasId, instanceInfo, *mesh, true, false, isSkeletalModel);
        }
    }
    
    for (const auto& mesh : model.m_OpaqueMeshes) 
        createGeometry(blasId, instanceInfo, *mesh, true, false, isSkeletalModel);
    
    for (const auto& mesh : model.m_TransparentMeshes) 
        createGeometry(blasId, instanceInfo, *mesh, false, false, isSkeletalModel);
    
    for (const auto& mesh : model.m_PunchThroughMeshes) 
        createGeometry(blasId, instanceInfo, *mesh, false, true, isSkeletalModel);
}

static size_t createBottomLevelAS(hh::mr::CTerrainModelData& terrainModel)
{
    const auto result = searchDatabaseData(terrainModel);

    if (!result.shouldCreate)
        return result.id;

    group.run([&terrainModel, result]
    {
        createBottomLevelAS(terrainModel, result.id, NULL, false);

        const auto msg = msgSender.start<MsgCreateBottomLevelAS>();
        msg->bottomLevelAS = result.id;
        msg->instanceInfo = 0;
        msg->matrixNum = 0;
        msgSender.finish();
     });

    return result.id;
}

static std::pair<size_t, size_t> createBottomLevelAS(const hh::mr::CSingleElement& singleElement)
{
    const auto result = searchDatabaseData(*singleElement.m_spModel);
    const bool isSkeletalModel = singleElement.m_spInstanceInfo->m_spAnimationPose != nullptr && singleElement.m_spModel->m_NodeNum > 1;
    const size_t instanceInfo = isSkeletalModel ? (size_t)singleElement.m_spInstanceInfo.get() : 0;

    if (result.shouldCreate || isSkeletalModel)
    {
        group.run([&singleElement, result, instanceInfo, isSkeletalModel] 
        {
            createBottomLevelAS(*singleElement.m_spModel, result.id, instanceInfo, isSkeletalModel);

            const size_t matrixNum = isSkeletalModel ? singleElement.m_spModel->m_NodeNum : 0;

            const auto msg = msgSender.start<MsgCreateBottomLevelAS>(matrixNum * 64);
            msg->bottomLevelAS = result.id;
            msg->instanceInfo = instanceInfo;
            msg->matrixNum = matrixNum;

            if (isSkeletalModel)
                memcpy(MSG_DATA_PTR(msg), singleElement.m_spInstanceInfo->m_spAnimationPose->GetMatrixList(), matrixNum * 64);

            msgSender.finish();
        });
    }

    return std::make_pair(result.id, instanceInfo);
}

static void traverse(const hh::mr::CRenderable* renderable, int instanceMask)
{
    if (!renderable->m_Enabled)
        return;

    group.run([renderable, instanceMask]
    {
        if (auto singleElement = dynamic_cast<const hh::mr::CSingleElement*>(renderable))
        {
            if (!singleElement->m_spModel->IsMadeAll() || (singleElement->m_spInstanceInfo->m_Flags & hh::mr::eInstanceInfoFlags_Invisible) != 0)
            {
                std::lock_guard lock(matrixMutex);
                prevMatrixMap.erase(singleElement);

                return;
            }

            const auto idPair = createBottomLevelAS(*singleElement);

            const auto msg = msgSender.start<MsgCreateInstance>();
            {
                std::lock_guard lock(matrixMutex);

                const auto matrixPair = prevMatrixMap.find(singleElement);

                for (size_t i = 0; i < 3; i++)
                {
                    for (size_t j = 0; j < 4; j++)
                    {
                        msg->transform[i][j] = singleElement->m_spInstanceInfo->m_Transform(i, j);

                        msg->prevTransform[i][j] = matrixPair == prevMatrixMap.end() ? 
                            singleElement->m_spInstanceInfo->m_Transform(i, j) : matrixPair->second(i, j);
                    }
                }

                prevMatrixMap[singleElement] = singleElement->m_spInstanceInfo->m_Transform;
            }

            msg->bottomLevelAS = idPair.first;
            msg->instanceInfo = idPair.second;
            msg->instanceMask = instanceMask;

            msgSender.finish();
        }
        else if (auto bundle = dynamic_cast<const hh::mr::CBundle*>(renderable))
        {
            for (const auto& it : bundle->m_RenderableList)
                traverse(it.get(), instanceMask);
        }
        else if (auto optimalBundle = dynamic_cast<const hh::mr::COptimalBundle*>(renderable))
        {
            for (const auto it : optimalBundle->m_RenderableList)
                traverse(it, instanceMask);
        }
    });
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

    // Set sky parameters
    {
        hh::mr::CRenderingDevice* renderingDevice = **(hh::mr::CRenderingDevice***)((char*)A1 + 16);

        const float backGroundScale[] = 
        {
            *(float*)0x1A489F0,
            0.0f,
            0.0f,
            0.0f
        };

        const float skyParam[] =
        {
            1.0f,
            *(float*)0x1E5E228,
            1.0f,
            1.0f
        };

        renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(67, backGroundScale, 1);
        renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(219, skyParam, 1);
    }

    identifierMutex.lock();

    tbb::parallel_for_each(identifierMap.begin(), identifierMap.end(), [](const auto& pair)
    {
        auto material = dynamic_cast<hh::mr::CMaterialData*>(pair.first);
        if (material)
            createMaterial(*material, pair.second);
    });

    identifierMutex.unlock();

    auto& renderScene = world->m_pMember->m_spRenderScene;

    const hh::base::CStringSymbol symbols[] = {"Sky", "Object", "Object_Overlay", "Player"};

    for (const auto symbol : symbols)
    {
        const auto pair = renderScene->m_BundleMap.find(symbol);
        if (pair == renderScene->m_BundleMap.end())
            continue;

        traverse(pair->second.get(), symbol.m_pSymbolNode == symbols[0].m_pSymbolNode ? INSTANCE_MASK_SKY : INSTANCE_MASK_DEFAULT);
    }

    std::lock_guard lock(instanceMutex);

    for (const auto instance : instanceSet)
    {
        group.run([instance]
        {
            if (!instance->IsMadeAll() || !instance->m_spTerrainModel)
                return;

            size_t blasId = createBottomLevelAS(*instance->m_spTerrainModel);

            const auto msg = msgSender.start<MsgCreateInstance>();

            for (size_t i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    msg->transform[i][j] = (*instance->m_scpTransform)(i, j);
                    msg->prevTransform[i][j] = (*instance->m_scpTransform)(i, j);
                }
            }

            memcpy(msg->prevTransform, instance->m_scpTransform.get(), sizeof(msg->prevTransform));

            msg->bottomLevelAS = blasId;
            msg->instanceInfo = 0;
            msg->instanceMask = INSTANCE_MASK_DEFAULT;

            msgSender.finish();
        });
    }

    group.wait();

    msgSender.oneShot<MsgNotifySceneTraversed>();
}

static void convertToTriangles(const hh::mr::CMeshData& mesh, hl::hh::mirage::raw_mesh* data)
{
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

    if (indices.empty())
        return;

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

    std::lock_guard lock(indexMutex);
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

    std::lock_guard lock(instanceMutex);
    instanceSet.insert(instance.get());
}

HOOK(hh::db::CDatabaseData*, __fastcall, ConstructDatabaseData, 0x699380, hh::db::CDatabaseData* This)
{
    setMadeForBridge(*This, false);
    return originalConstructDatabaseData(This);
}

HOOK(void, __fastcall, DestructDatabaseData, 0x6993B0, hh::db::CDatabaseData* This)
{
    if (isMadeForBridge(*This))
    {
        group.wait();

        std::lock_guard lock(identifierMutex);
        const auto idPair = identifierMap.find(This);

        if (idPair != identifierMap.end())
        {
            const auto msg = msgSender.start<MsgReleaseResource>();
            msg->resource = idPair->second;
            msgSender.finish();

            identifierMap.erase(idPair);
        }
    }

    originalDestructDatabaseData(This);
}

HOOK(void, __fastcall, DestructMesh, 0x7227A0, hh::mr::CMeshData* This)
{
    {
        std::lock_guard lock(indexMutex);
        indexMap.erase(This);
    }

    originalDestructMesh(This);
}

HOOK(void, __fastcall, DestructTerrainInstanceInfo, 0x717090, hh::mr::CTerrainInstanceInfoData* This)
{
    {
        std::lock_guard lock(instanceMutex);
        instanceSet.erase(This);
    }

    originalDestructTerrainInstanceInfo(This);
}

HOOK(void, __fastcall, DestructSingleElement, 0x701850, hh::mr::CSingleElement* This)
{
    if (This->m_spModel != nullptr && This->m_spInstanceInfo != nullptr)
    {
        {
            std::lock_guard lock(identifierMutex);
            const auto idPair = identifierMap.find(This->m_spModel.get());

            if (idPair != identifierMap.end())
            {
                const auto msg = msgSender.start<MsgReleaseInstanceInfo>();
                msg->bottomLevelAS = idPair->second;
                msg->instanceInfo = (unsigned int)This->m_spInstanceInfo.get();
                msgSender.finish();
            }
        }

        {
            std::lock_guard lock(matrixMutex);
            prevMatrixMap.erase(This);
        }
    }

    originalDestructSingleElement(This);
}

void RaytracingManager::init()
{
    shaderMapping.load("ShaderMapping.bin");

    WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2

    WRITE_MEMORY(0x13DC2D8, void*, &SceneRender_Raytracing); // Override scene render function
    WRITE_MEMORY(0x13DDCA0, size_t, 1); // Override game scene child count

    WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
    WRITE_MEMORY(0x722760, uint8_t, 0xC3); // Disable shared buffer

    INSTALL_HOOK(MakeMesh); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeMesh2); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeTerrainInstanceInfo); // Capture instances to put to TLAS

    INSTALL_HOOK(ConstructDatabaseData); // Made for bridge flag
    INSTALL_HOOK(DestructDatabaseData); // Garbage collection

    INSTALL_HOOK(DestructMesh); // Garbage collection
    INSTALL_HOOK(DestructTerrainInstanceInfo); // Garbage collection

    INSTALL_HOOK(DestructSingleElement); // Garbage collection
}