#include "RaytracingManager.h"

#include "Buffer.h"
#include "Identifier.h"
#include "Message.h"
#include "MessageSender.h"
#include "Profiler.h"
#include "RaytracingParams.h"
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
                strcpy(msg->shader, pair->first.c_str());

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

static void createMesh(
    const size_t modelId,
    const hh::mr::CSingleElement* element,
    const size_t elementId,
    bool isSky,
    const hh::mr::CMeshData& mesh,
    bool isOpaque, 
    bool isPunchThrough)
{
    if (mesh.m_VertexNum == 0 || mesh.m_IndexNum <= 2)
        return;

    size_t materialId = 0;

    if (element != nullptr)
    {
        const auto pair = element->m_MaterialMap.find(mesh.m_spMaterial.get());

        if (pair != element->m_MaterialMap.end())
            materialId = createMaterial(*pair->second);
    }

    if (materialId == 0)
        materialId = createMaterial(*mesh.m_spMaterial);

    indexMutex.lock();
    auto& indexBuffer = indexMap[&mesh];
    indexMutex.unlock();

    if (!indexBuffer.first)
        return;

    const auto msg = msgSender.start<MsgCreateMesh>(element != nullptr ? mesh.m_NodeNum : 0);

    msg->model = modelId;
    msg->element = elementId;

    if (isSky)
    {
        msg->group = INSTANCE_MASK_SKY;
    }
    else
    {
        if (!isOpaque && !isPunchThrough)
            msg->group = INSTANCE_MASK_TRANS_WATER;
        else
            msg->group = INSTANCE_MASK_OPAQ_PUNCH;
    }

    msg->opaque = isOpaque;
    msg->punchThrough = isPunchThrough;
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

    if (element != nullptr)
    {
        msg->nodeNum = mesh.m_NodeNum;
        memcpy(MSG_DATA_PTR(msg), mesh.m_pNodeIndices, mesh.m_NodeNum);
    }
    else
    {
        msg->nodeNum = 0;
    }

    msgSender.finish();
}

template<typename T>
static void createModel(
    T& model,
    size_t modelId,
    const hh::mr::CSingleElement* element, 
    size_t elementId, 
    bool isSky)
{
    for (const auto& meshGroup : model.m_NodeGroupModels)
    {
        if (!meshGroup->m_Visible)
            continue;
    
        for (const auto& mesh : meshGroup->m_OpaqueMeshes)
            createMesh(modelId, element, elementId, isSky, *mesh, true, false);
    
        for (const auto& mesh : meshGroup->m_TransparentMeshes) 
            createMesh(modelId, element, elementId, isSky, *mesh, false, false);
    
        for (const auto& mesh : meshGroup->m_PunchThroughMeshes) 
            createMesh(modelId, element, elementId, isSky, *mesh, false, true);
    
        for (const auto& specialMeshGroup : meshGroup->m_SpecialMeshGroups)
        {
            for (const auto& mesh : specialMeshGroup)
                createMesh(modelId, element, elementId, isSky, *mesh, false, false);
        }
    }
    
    for (const auto& mesh : model.m_OpaqueMeshes) 
        createMesh(modelId, element, elementId, isSky, *mesh, true, false);
    
    for (const auto& mesh : model.m_TransparentMeshes) 
        createMesh(modelId, element, elementId, isSky, *mesh, false, false);
    
    for (const auto& mesh : model.m_PunchThroughMeshes) 
        createMesh(modelId, element, elementId, isSky, *mesh, false, true);
}

static size_t createModel(hh::mr::CTerrainModelData& terrainModel)
{
    const auto result = searchDatabaseData(terrainModel);

    if (!result.shouldCreate)
        return result.id;

    group.run([&terrainModel, result]
    {
        createModel(
            terrainModel,
            result.id, 
            nullptr,
            NULL, 
            false);

        const auto msg = msgSender.start<MsgCreateModel>();

        msg->model = result.id;
        msg->element = 0;
        msg->matrixNum = 0;

        msgSender.finish();
     });

    return result.id;
}

static std::pair<size_t, size_t> createModel(const hh::mr::CSingleElement& element, bool isSky)
{
    const auto result = searchDatabaseData(*element.m_spModel);

    const bool isUnique = !element.m_MaterialMap.empty() || 
        element.m_spInstanceInfo->m_spAnimationPose != nullptr;

    const size_t elementId = isUnique ? (size_t)&element : NULL;

    if (result.shouldCreate || isUnique)
    {
        group.run([&element, isSky, result, isUnique, elementId]
        {
            createModel(
                *element.m_spModel, 
                result.id, 
                isUnique ? &element : nullptr,
                elementId,
                isSky);

            const size_t matrixNum = isUnique && element.m_spInstanceInfo->m_spAnimationPose != nullptr ? element.m_spModel->m_NodeNum : 0;

            const auto msg = msgSender.start<MsgCreateModel>(matrixNum * 64);
            msg->model = result.id;
            msg->element = elementId;
            msg->matrixNum = matrixNum;

            if (matrixNum != 0)
                memcpy(MSG_DATA_PTR(msg), element.m_spInstanceInfo->m_spAnimationPose->GetMatrixList(), matrixNum * 64);

            msgSender.finish();
        });
    }

    return std::make_pair(result.id, elementId);
}

static void traverse(const hh::mr::CRenderable* renderable, bool isSky)
{
    if (!renderable->m_Enabled)
        return;

    group.run([renderable, isSky]
    {
        if (auto element = dynamic_cast<const hh::mr::CSingleElement*>(renderable))
        {
            if (!element->m_spModel->IsMadeAll() || (element->m_spInstanceInfo->m_Flags & hh::mr::eInstanceInfoFlags_Invisible) != 0)
            {
                std::lock_guard lock(matrixMutex);
                prevMatrixMap.erase(element);

                return;
            }

            const auto idPair = createModel(*element, isSky);

            const auto msg = msgSender.start<MsgCreateInstance>();
            {
                std::lock_guard lock(matrixMutex);

                const auto matrixPair = prevMatrixMap.find(element);

                for (size_t i = 0; i < 3; i++)
                {
                    for (size_t j = 0; j < 4; j++)
                    {
                        msg->transform[i][j] = element->m_spInstanceInfo->m_Transform(i, j);

                        msg->prevTransform[i][j] = matrixPair == prevMatrixMap.end() ? 
                            element->m_spInstanceInfo->m_Transform(i, j) : matrixPair->second(i, j);
                    }
                }

                prevMatrixMap[element] = element->m_spInstanceInfo->m_Transform;
            }

            msg->model = idPair.first;
            msg->element = idPair.second;

            msgSender.finish();
        }
        else if (auto bundle = dynamic_cast<const hh::mr::CBundle*>(renderable))
        {
            for (const auto& it : bundle->m_RenderableList)
                traverse(it.get(), isSky);
        }
        else if (auto optimalBundle = dynamic_cast<const hh::mr::COptimalBundle*>(renderable))
        {
            for (const auto it : optimalBundle->m_RenderableList)
                traverse(it, isSky);
        }
    });
}

static bool prevRaytracingEnable;
static bool initToggleablePatchesAndReturnShouldRender();

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

    const bool prevEnable = prevRaytracingEnable;

    if (!initToggleablePatchesAndReturnShouldRender())
        return;

    PROFILER("Send Raytracing Render Messages");

    bool resetAccumulation = RaytracingParams::update();
    resetAccumulation |= prevEnable != RaytracingParams::enable;

    hh::mr::CRenderingDevice* renderingDevice = **(hh::mr::CRenderingDevice***)((char*)A1 + 16);

    // Set sky parameters
    {
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

    static boost::shared_ptr<hh::mr::CLightData> light;
    const boost::shared_ptr<hh::mr::CLightData> prevLight = light;

    // Set light parameters
    if (RaytracingParams::light != nullptr && RaytracingParams::light->IsMadeAll())
    {
        light = RaytracingParams::light;

        renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(183, light->m_Direction.data(), 1);
        renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(184, light->m_Color.data(), 1);
        renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(185, light->m_Color.data(), 1);

        renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(10, light->m_Direction.data(), 1);
        renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(36, light->m_Color.data(), 1);
        renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(37, light->m_Color.data(), 1);
    }
    resetAccumulation |= light != prevLight;

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

    for (size_t i = RaytracingParams::sky != nullptr && RaytracingParams::sky->IsMadeAll()  ? 1u : 0u; i < _countof(symbols); i++)
    {
        const auto pair = renderScene->m_BundleMap.find(symbols[i]);
        if (pair == renderScene->m_BundleMap.end())
            continue;

        traverse(pair->second.get(), i == 0);
    }

    static boost::shared_ptr<hh::mr::CSingleElement> singleElement;
    const boost::shared_ptr<hh::mr::CSingleElement> prevSingleElement = singleElement;

    if (RaytracingParams::sky != nullptr && RaytracingParams::sky->IsMadeAll())
    {
        if (!singleElement || singleElement->m_spModel != RaytracingParams::sky)
            singleElement = boost::make_shared<hh::mr::CSingleElement>(RaytracingParams::sky);

        traverse(singleElement.get(), true);
    }
    resetAccumulation |= singleElement != prevSingleElement;

    std::lock_guard lock(instanceMutex);

    for (const auto instance : instanceSet)
    {
        group.run([instance]
        {
            if (!instance->IsMadeAll() || !instance->m_spTerrainModel)
                return;

            const size_t modelId = createModel(*instance->m_spTerrainModel);

            const auto msg = msgSender.start<MsgCreateInstance>();

            for (size_t i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    msg->transform[i][j] = (*instance->m_scpTransform)(i, j);
                    msg->prevTransform[i][j] = (*instance->m_scpTransform)(i, j);
                }
            }

            msg->model = modelId;
            msg->element = 0;

            msgSender.finish();
        });
    }

    group.wait();

    auto msg = msgSender.start<MsgNotifySceneTraversed>();
    msg->resetAccumulation = resetAccumulation ? 1 : 0;
    msgSender.finish();
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
                const auto msg = msgSender.start<MsgReleaseElement>();
                msg->model = idPair->second;
                msg->element = (unsigned int)This;
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

static bool initToggleablePatchesAndReturnShouldRender()
{
    if (prevRaytracingEnable == RaytracingParams::enable)
        return RaytracingParams::enable;

    if (RaytracingParams::enable)
    {
        WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
        WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
        WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
        WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2
        WRITE_MEMORY(0x13DDCA0, size_t, 1); // Override game scene child count
        //WRITE_MEMORY(0x13E01A8, size_t, 0); // Override motion blur child count
        WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
    }
    else
    {
        WRITE_MEMORY(0x13DDFD8, size_t, 4); // Enable shadow map
        WRITE_MEMORY(0x13DDB20, size_t, 1); // Enable sky render
        WRITE_MEMORY(0x13DDBA0, size_t, 22); // Enable reflection map 1
        WRITE_MEMORY(0x13DDC20, size_t, 22); // Enable reflection map 2
        WRITE_MEMORY(0x13DDCA0, size_t, 22); // Restore game scene child count
        //WRITE_MEMORY(0x13E01A8, size_t, 2); // Restore motion blur child count
        WRITE_MEMORY(0x72ACD0, uint8_t, 0x53, 0x56, 0x57); // Enable GI texture
    }

    const bool shouldRender = prevRaytracingEnable;
    prevRaytracingEnable = RaytracingParams::enable;
    return shouldRender;
}

HOOK(void, __cdecl, SceneTraversed_ScreenMotionBlurFilter, 0x6577C0, size_t a1, size_t a2)
{
    if (RaytracingParams::enable)
    {
        const auto msg = msgSender.start<MsgCopyVelocityMap>();
        msg->enableBoostBlur = *(bool*)(*(size_t*)(a1 + 4) + 977) ? 1u : 0u;
        msgSender.finish();
    }
    else
    {
        originalSceneTraversed_ScreenMotionBlurFilter(a1, a2);
    }
}

void RaytracingManager::init()
{
    shaderMapping.load("ShaderMapping.bin");

    initToggleablePatchesAndReturnShouldRender();

    WRITE_MEMORY(0x13DC2D8, void*, &SceneRender_Raytracing); // Override scene render function
    WRITE_MEMORY(0x722760, uint8_t, 0xC3); // Disable shared buffer

    INSTALL_HOOK(MakeMesh); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeMesh2); // Convert triangle strips to triangles for BLAS creation
    INSTALL_HOOK(MakeTerrainInstanceInfo); // Capture instances to put to TLAS

    INSTALL_HOOK(ConstructDatabaseData); // Made for bridge flag
    INSTALL_HOOK(DestructDatabaseData); // Garbage collection

    INSTALL_HOOK(DestructMesh); // Garbage collection
    INSTALL_HOOK(DestructTerrainInstanceInfo); // Garbage collection

    INSTALL_HOOK(DestructSingleElement); // Garbage collection

    //INSTALL_HOOK(SceneTraversed_ScreenMotionBlurFilter);
}