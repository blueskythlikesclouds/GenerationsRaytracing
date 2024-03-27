#include "ModelData.h"

#include "GeometryFlags.h"
#include "IndexBuffer.h"
#include "InstanceData.h"
#include "MaterialData.h"
#include "MeshData.h"
#include "Message.h"
#include "MessageSender.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"

template<typename T>
static void traverseMeshGroup(const hh::vector<boost::shared_ptr<Hedgehog::Mirage::CMeshData>>& meshGroup, uint32_t flags, const T& function)
{
    for (const auto& meshData : meshGroup)
    {
        const auto& meshDataEx = *reinterpret_cast<const MeshDataEx*>(meshData.get());

        if (meshDataEx.m_spMaterial != nullptr && meshDataEx.m_indexCount > 2 && meshDataEx.m_VertexNum > 2 &&
            meshDataEx.m_indices != nullptr && meshDataEx.m_pD3DVertexBuffer != nullptr)
        {
            function(meshDataEx, flags);
        }
    }
}

template<typename TModelData, typename TFunction>
static void traverseModelData(const TModelData& modelData, const TFunction& function)
{
    traverseMeshGroup(modelData.m_OpaqueMeshes, NULL, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_OpaqueMeshes, NULL, function);
    }

    traverseMeshGroup(modelData.m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_PunchThroughMeshes, GEOMETRY_FLAG_PUNCH_THROUGH, function);
    }

    traverseMeshGroup(modelData.m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (nodeGroupModelData->m_Visible)
            traverseMeshGroup(nodeGroupModelData->m_TransparentMeshes, GEOMETRY_FLAG_TRANSPARENT, function);
    }

    for (const auto& nodeGroupModelData : modelData.m_NodeGroupModels)
    {
        if (!nodeGroupModelData->m_Visible)
            continue;

        for (const auto& specialMeshGroup : nodeGroupModelData->m_SpecialMeshGroups)
            traverseMeshGroup(specialMeshGroup, GEOMETRY_FLAG_TRANSPARENT, function);
    }
}

template<typename T>
static void createBottomLevelAccelStruct(const T& modelData, uint32_t& bottomLevelAccelStructId, uint32_t poseVertexBufferId)
{
    assert(bottomLevelAccelStructId == NULL);

    size_t geometryCount = 0;

    traverseModelData(modelData, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

    if (geometryCount == 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgCreateBottomLevelAccelStruct>(
        geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    message.bottomLevelAccelStructId = (bottomLevelAccelStructId = ModelData::s_idAllocator.allocate());
    message.preferFastBuild = poseVertexBufferId != NULL;
    memset(message.data, 0, geometryCount * sizeof(MsgCreateBottomLevelAccelStruct::GeometryDesc));

    auto geometryDesc = reinterpret_cast<MsgCreateBottomLevelAccelStruct::GeometryDesc*>(message.data);
    uint32_t poseVertexOffset = 0;

    traverseModelData(modelData, [&](const MeshDataEx& meshDataEx, uint32_t flags)
    {
        if (poseVertexBufferId != NULL)
            flags |= GEOMETRY_FLAG_POSE;

        geometryDesc->flags = flags;
        geometryDesc->indexBufferId = meshDataEx.m_indices->getId();
        geometryDesc->indexCount = meshDataEx.m_indexCount;

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
        geometryDesc->vertexCount = meshDataEx.m_VertexNum;
        geometryDesc->vertexOffset = vertexOffset;

        const auto vertexDeclaration = reinterpret_cast<const VertexDeclaration*>(
            meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration);

        auto vertexElement = vertexDeclaration->getVertexElements();
        uint32_t texCoordOffsets[4]{};

        while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
        {
            switch (vertexElement->Usage)
            {
            case D3DDECLUSAGE_NORMAL:
                geometryDesc->normalOffset = vertexElement->Offset;
                break;

            case D3DDECLUSAGE_TANGENT:
                geometryDesc->tangentOffset = vertexElement->Offset;
                break;

            case D3DDECLUSAGE_BINORMAL:
                geometryDesc->binormalOffset = vertexElement->Offset;
                break;

            case D3DDECLUSAGE_TEXCOORD:
                assert(vertexElement->UsageIndex < 4);
                texCoordOffsets[vertexElement->UsageIndex] = vertexElement->Offset;
                break;

            case D3DDECLUSAGE_COLOR:
                geometryDesc->colorOffset = vertexElement->Offset;
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
            materialDataEx->m_materialId = MaterialData::s_idAllocator.allocate();

        geometryDesc->materialId = materialDataEx->m_materialId;

        ++geometryDesc;
    });

    assert(reinterpret_cast<uint8_t*>(geometryDesc - geometryCount) == message.data);

    s_messageSender.endMessage();
}

HOOK(TerrainModelDataEx*, __fastcall, TerrainModelDataConstructor, 0x717440, TerrainModelDataEx* This)
{
    const auto result = originalTerrainModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;

    return result;
}

HOOK(void, __fastcall, TerrainModelDataDestructor, 0x717230, TerrainModelDataEx* This)
{
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);
    originalTerrainModelDataDestructor(This);
}

HOOK(ModelDataEx*, __fastcall, ModelDataConstructor, 0x4FA400, ModelDataEx* This)
{
    const auto result = originalModelDataConstructor(This);

    This->m_bottomLevelAccelStructId = NULL;
    new (&This->m_noAoModel) boost::shared_ptr<Hedgehog::Mirage::CModelData>();

    return result;
}

HOOK(void, __fastcall, ModelDataDestructor, 0x4FA520, ModelDataEx* This)
{
    This->m_noAoModel.~shared_ptr();
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);

    originalModelDataDestructor(This);
}

void ModelData::createBottomLevelAccelStruct(TerrainModelDataEx& terrainModelDataEx)
{
    if (terrainModelDataEx.m_bottomLevelAccelStructId == NULL)
        ::createBottomLevelAccelStruct(terrainModelDataEx, terrainModelDataEx.m_bottomLevelAccelStructId, NULL);
}

static boost::shared_ptr<Hedgehog::Mirage::CMaterialData> cloneMaterial(const Hedgehog::Mirage::CMaterialData& material)
{
    const auto materialClone = static_cast<Hedgehog::Mirage::CMaterialData*>(__HH_ALLOC(sizeof(MaterialDataEx)));
    Hedgehog::Mirage::fpCMaterialDataCtor(materialClone);

    static Hedgehog::Base::CSharedString s_materialCloneName("RaytracingMaterialClone");

    materialClone->m_TypeAndName = s_materialCloneName;
    materialClone->m_Flags = Hedgehog::Database::eDatabaseDataFlags_IsMadeOne | Hedgehog::Database::eDatabaseDataFlags_IsMadeAll;

    static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");

    if (material.m_spTexsetData != nullptr)
    {
        const auto texsetClone = boost::make_shared<Hedgehog::Mirage::CTexsetData>();
        texsetClone->m_Flags = Hedgehog::Database::eDatabaseDataFlags_IsMadeOne | Hedgehog::Database::eDatabaseDataFlags_IsMadeAll;

        bool shouldClone = true;

        texsetClone->m_TextureList.reserve(material.m_spTexsetData->m_TextureList.size());
        for (const auto& texture : material.m_spTexsetData->m_TextureList)
        {
            if (texture->m_Type == s_diffuseSymbol && shouldClone)
            {
                auto texClone = boost::make_shared<Hedgehog::Mirage::CTextureData>();

                if (texture->m_spPictureData != nullptr)
                {
                    auto picClone = boost::make_shared<Hedgehog::Mirage::CPictureData>();
                    *picClone = *texture->m_spPictureData;

                    if (picClone->m_pD3DTexture != nullptr)
                        picClone->m_pD3DTexture->AddRef();

                    texClone->m_spPictureData = picClone;
                }

                texClone->m_TexcoordIndex = texture->m_TexcoordIndex;
                texClone->m_SamplerState = texture->m_SamplerState;
                texClone->m_Type = texture->m_Type;

                texsetClone->m_TextureList.push_back(texClone);

                shouldClone = false;
            }
            else
            {
                texsetClone->m_TextureList.push_back(texture);
            }
        }

        texsetClone->m_TextureNameList = material.m_spTexsetData->m_TextureNameList;

        materialClone->m_spTexsetData = texsetClone;
    }

    materialClone->m_spShaderListData = material.m_spShaderListData;
    materialClone->m_Int4Params = material.m_Int4Params;

    static Hedgehog::Base::CStringSymbol s_float4ParamsToClone[] =
    {
        s_diffuseSymbol,
        "ambient",
        "specular",
        "emissive",
        "power_gloss_level",
        "opacity_reflection_refraction_spectype",
    };

    materialClone->m_Float4Params.reserve(material.m_Float4Params.size());
    for (const auto& float4Param : material.m_Float4Params)
    {
        bool shouldClone = false;
        for (const auto float4Symbol : s_float4ParamsToClone)
        {
            if (float4Param->m_Name == float4Symbol)
            {
                shouldClone = true;
                break;
            }
        }

        if (shouldClone)
        {
            const auto float4ParamClone = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();

            float4ParamClone->m_Name = float4Param->m_Name;
            float4ParamClone->m_ValueNum = float4Param->m_ValueNum;
            float4ParamClone->m_spValue = boost::make_shared<float[]>(4 * float4Param->m_ValueNum);

            memcpy(float4ParamClone->m_spValue.get(), float4Param->m_spValue.get(), float4Param->m_ValueNum * sizeof(float[4]));

            materialClone->m_Float4Params.push_back(float4ParamClone);
        }
        else
        {
            materialClone->m_Float4Params.push_back(float4Param);
        }
    }

    materialClone->m_Bool4Params = material.m_Bool4Params;
    materialClone->m_AlphaThreshold = material.m_AlphaThreshold;
    materialClone->m_DoubleSided = material.m_DoubleSided;
    materialClone->m_Additive = material.m_Additive;
    materialClone->m_MaterialFlags = material.m_MaterialFlags;
    
    return boost::shared_ptr<Hedgehog::Mirage::CMaterialData>(materialClone);
}

static boost::shared_ptr<Hedgehog::Mirage::CParameterFloat4Element> createFloat4Param(
    Hedgehog::Mirage::CMaterialData& material, Hedgehog::Base::CStringSymbol name, uint32_t valueNum, float* value)
{
    for (const auto& float4Param : material.m_Float4Params)
    {
        if (float4Param->m_Name == name && float4Param->m_ValueNum == valueNum)
        {
            if (value != nullptr)
                memcpy(float4Param->m_spValue.get(), value, valueNum * sizeof(float[4]));

            return float4Param;
        }
    }

    const auto float4Param = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();
    float4Param->m_Name = name;
    float4Param->m_ValueNum = valueNum;
    float4Param->m_spValue = boost::make_shared<float[]>(valueNum * 4, 0.0f);

    if (value != nullptr)
        memcpy(float4Param->m_spValue.get(), value, valueNum * sizeof(float[4]));
    
    material.m_Float4Params.push_back(float4Param);
    return float4Param;
}

void ModelData::processEyeMaterials(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, MaterialMap& materialMap)
{
    if (!instanceInfoEx.m_handledEyeMaterials)
    {
        if (modelDataEx.m_spNodes != nullptr)
        {
            for (size_t i = 0; i < modelDataEx.m_NodeNum; i++)
            {
                if (modelDataEx.m_spNodes[i].m_Name == "Head")
                {
                    instanceInfoEx.m_headNodeIndex = i;
                    break;
                }
            }
        }

        static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightPositionSymbol("g_SonicEyeHighLightPosition");
        static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightPositionRaytracingSymbol("g_SonicEyeHighLightPositionRaytracing");

        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            if (meshDataEx.m_spMaterial == nullptr || meshDataEx.m_spMaterial->m_spShaderListData == nullptr || 
                strstr(meshDataEx.m_spMaterial->m_spShaderListData->m_TypeAndName.c_str(), "ChrEye") == nullptr)
            {
                return;
            }

            auto& materialOverride = materialMap[meshDataEx.m_spMaterial.get()];
            if (materialOverride == nullptr)
                materialOverride = cloneMaterial(*meshDataEx.m_spMaterial);

            auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(materialOverride.get());

            if (materialDataEx.m_eyeParamHolder == nullptr)
            {
                materialDataEx.m_eyeParamHolder = std::make_unique<EyeParamHolder>();

                materialDataEx.m_eyeParamHolder->originalValue = 
                    createFloat4Param(materialDataEx, s_sonicEyeHighLightPositionSymbol, 1, nullptr)->m_spValue;

                if (materialDataEx.m_spShaderListData->m_TypeAndName == "Mirage.shader-list ChrEyeFHL")
                {
                    materialDataEx.m_eyeParamHolder->originalValue[0] = 0.0f;
                    materialDataEx.m_eyeParamHolder->originalValue[1] = 0.0f;
                    materialDataEx.m_eyeParamHolder->originalValue[2] = 1.0f;
                    materialDataEx.m_eyeParamHolder->originalValue[3] = 0.0f;
                }
                else
                {
                    materialDataEx.m_eyeParamHolder->originalValue[3] = 1.0f;
                }

                materialDataEx.m_eyeParamHolder->raytracingValue = 
                    createFloat4Param(materialDataEx, s_sonicEyeHighLightPositionRaytracingSymbol, 1, nullptr)->m_spValue;
            }
        });

        instanceInfoEx.m_handledEyeMaterials = true;
    }
}

static std::unordered_map<Hedgehog::Mirage::CMaterialData*, float*> s_tmpMaterialCnt;

static void processTexcoordMotion(MaterialMap& materialMap, const Hedgehog::Motion::CTexcoordMotion& texcoordMotion)
{
    if (texcoordMotion.m_pMaterialData != nullptr && texcoordMotion.m_pMaterialData->IsMadeAll())
    {
        static Hedgehog::Base::CStringSymbol s_texCoordOffsetSymbol("mrgTexcoordOffset");

        const auto materialDataEx = reinterpret_cast<MaterialDataEx*>(texcoordMotion.m_pMaterialData);
        const auto materialData = materialDataEx->m_fhlMaterial != nullptr ? materialDataEx->m_fhlMaterial.get() : materialDataEx;

        auto& materialClone = materialMap[materialData];
        if (materialClone == nullptr)
            materialClone = cloneMaterial(*materialData);

        auto& offsets = s_tmpMaterialCnt[materialData];
        if (offsets == nullptr)
        {
            offsets = createFloat4Param(*materialClone, s_texCoordOffsetSymbol, 2, nullptr)->m_spValue.get();
            std::fill_n(offsets, 8, 0.0f);
        }

        if ((texcoordMotion.m_Field4 & 0x2) == 0)
        {
            for (size_t i = 0; i < 8; i++)
                offsets[i] += texcoordMotion.m_TexcoordOffset[i];
        }
    }
}

static void processMaterialMotion(MaterialMap& materialMap, const Hedgehog::Motion::CMaterialMotion& materialMotion)
{
    if (materialMotion.m_pMaterialData != nullptr && materialMotion.m_pMaterialData->IsMadeAll())
    {
        static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");
        static Hedgehog::Base::CStringSymbol s_ambientSymbol("ambient");
        static Hedgehog::Base::CStringSymbol s_specularSymbol("specular");
        static Hedgehog::Base::CStringSymbol s_emissiveSymbol("emissive");
        static Hedgehog::Base::CStringSymbol s_powerGlossLevelSymbol("power_gloss_level");
        static Hedgehog::Base::CStringSymbol s_opacityReflectionRefractionSpectypeSymbol("opacity_reflection_refraction_spectype");

        auto& material = materialMap[materialMotion.m_pMaterialData];
        if (material == nullptr)
            material = cloneMaterial(*materialMotion.m_pMaterialData);

        const auto& matMotionData = (materialMotion.m_Field4 & 0x2) == 0 ? 
            materialMotion.m_MaterialMotionData : materialMotion.m_DefaultMaterialMotionData;

        for (const auto& float4Param : material->m_Float4Params)
        {
            if (float4Param->m_Name == s_diffuseSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.Diffuse, sizeof(float[4]));

            else if (float4Param->m_Name == s_ambientSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.Ambient, sizeof(float[4]));
            
            else if (float4Param->m_Name == s_specularSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.Specular, sizeof(float[4]));
            
            else if (float4Param->m_Name == s_emissiveSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.Emissive, sizeof(float[4]));
            
            else if (float4Param->m_Name == s_powerGlossLevelSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.PowerGlossLevel, sizeof(float[4]));
            
            else if (float4Param->m_Name == s_opacityReflectionRefractionSpectypeSymbol)
                memcpy(float4Param->m_spValue.get(), matMotionData.OpacityReflectionRefractionSpectype, sizeof(float[4]));
        }
    }
}

static void processTexpatternMotion(MaterialMap& materialMap, const Hedgehog::Motion::CTexpatternMotion& texpatternMotion)
{
    if (texpatternMotion.m_pMaterialData != nullptr && texpatternMotion.m_pMaterialData->IsMadeAll() &&
        texpatternMotion.m_pPictureData != nullptr && texpatternMotion.m_pPictureData->IsMadeAll())
    {
        static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");

        auto& material = materialMap[texpatternMotion.m_pMaterialData];
        if (material == nullptr)
            material = cloneMaterial(*texpatternMotion.m_pMaterialData);

        if (material->m_spTexsetData != nullptr)
        {
            for (const auto& texture : material->m_spTexsetData->m_TextureList)
            {
                if (texture->m_Type == s_diffuseSymbol)
                {
                    if (texture->m_spPictureData != nullptr)
                    {
                        if (texture->m_spPictureData->m_pD3DTexture != nullptr)
                            texture->m_spPictureData->m_pD3DTexture->Release();

                        texture->m_spPictureData->m_pD3DTexture = texpatternMotion.m_pPictureData->m_pD3DTexture;
                        texture->m_spPictureData->m_pD3DTexture->AddRef();
                    }
                    break;
                }
            }
        }
    }
}

void ModelData::processSingleElementEffect(MaterialMap& materialMap, Hedgehog::Mirage::CSingleElementEffect* singleElementEffect)
{
    if (const auto singleElementEffectMotionAll = dynamic_cast<Hedgehog::Motion::CSingleElementEffectMotionAll*>(singleElementEffect))
    {
        for (const auto& texcoordMotion : singleElementEffectMotionAll->m_TexcoordMotionList)
            processTexcoordMotion(materialMap, texcoordMotion);

        for (const auto& materialMotion : singleElementEffectMotionAll->m_MaterialMotionList)
            processMaterialMotion(materialMap, materialMotion);

        for (const auto& texpatternMotion : singleElementEffectMotionAll->m_TexpatternMotionList)
            processTexpatternMotion(materialMap, texpatternMotion);

        s_tmpMaterialCnt.clear();
    }
    else if (const auto singleElementEffectUvMotion = dynamic_cast<Hedgehog::Motion::CSingleElementEffectUvMotion*>(singleElementEffect))
    {
        for (const auto& texcoordMotion : singleElementEffectUvMotion->m_TexcoordMotionList)
            processTexcoordMotion(materialMap, texcoordMotion);

        s_tmpMaterialCnt.clear();
    }
    else if (const auto singleElementEffectMatMotion = dynamic_cast<Hedgehog::Motion::CSingleElementEffectMatMotion*>(singleElementEffect))
    {
        for (const auto& materialMotion : singleElementEffectMatMotion->m_MaterialMotionList)
            processMaterialMotion(materialMap, materialMotion);
    }
}

void ModelData::createBottomLevelAccelStruct(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap)
{
    static Hedgehog::Base::CStringSymbol s_texCoordOffsetSymbol("mrgTexcoordOffset");

    for (auto& [key, value] : materialMap)
    {
        auto& keyEx = *reinterpret_cast<MaterialDataEx*>(key);
        if (keyEx.m_fhlMaterial != nullptr)
        {
            const auto findResult = materialMap.find(keyEx.m_fhlMaterial.get());
            if (findResult != materialMap.end())
            {
                for (const auto& float4Param : value->m_Float4Params)
                {
                    if (float4Param->m_Name == s_texCoordOffsetSymbol && float4Param->m_ValueNum == 2)
                    {
                        createFloat4Param(*findResult->second, s_texCoordOffsetSymbol, 2, float4Param->m_spValue.get());
                        break;
                    }
                }
            }
        }

        const auto& valueEx = *reinterpret_cast<MaterialDataEx*>(value.get());
        if (valueEx.m_eyeParamHolder != nullptr)
        {
            auto transform = instanceInfoEx.m_Transform;
            if (instanceInfoEx.m_spPose != nullptr && instanceInfoEx.m_spPose->GetMatrixNum() > instanceInfoEx.m_headNodeIndex)
                transform = transform * instanceInfoEx.m_spPose->GetMatrixList()[instanceInfoEx.m_headNodeIndex];

            *reinterpret_cast<Eigen::Array4f*>(valueEx.m_eyeParamHolder->raytracingValue.get()) = 
                transform * Eigen::Vector4f(*reinterpret_cast<Eigen::Array4f*>(valueEx.m_eyeParamHolder->originalValue.get()));
        }
    }

    for (auto& [key, value] : materialMap)
        MaterialData::create(*value, true);

    const bool shouldCheckForHash = modelDataEx.m_hashFrame != RaytracingRendering::s_frame;

    if (shouldCheckForHash)
    {
        const XXH32_hash_t modelHash = XXH32(modelDataEx.m_NodeGroupModels.data(),
            modelDataEx.m_NodeGroupModels.size() * sizeof(modelDataEx.m_NodeGroupModels[0]), 0);

        uint32_t visibilityBits = 0;
        for (size_t i = 0; i < modelDataEx.m_NodeGroupModels.size(); i++)
        {
            if (modelDataEx.m_NodeGroupModels[i]->m_Visible)
                visibilityBits |= 1 << i;
        }

        if (modelDataEx.m_modelHash != modelHash || modelDataEx.m_visibilityBits != visibilityBits)
            RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, modelDataEx.m_bottomLevelAccelStructId);

        modelDataEx.m_modelHash = modelHash;
        modelDataEx.m_visibilityBits = visibilityBits;
        modelDataEx.m_hashFrame = RaytracingRendering::s_frame;
    }

    if (instanceInfoEx.m_modelHash != modelDataEx.m_modelHash || instanceInfoEx.m_visibilityBits != modelDataEx.m_visibilityBits)
    {
        RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceInfoEx.m_instanceId);
        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, instanceInfoEx.m_bottomLevelAccelStructId);
        instanceInfoEx.m_poseVertexBuffer = nullptr;
    }

    instanceInfoEx.m_modelHash = modelDataEx.m_modelHash;
    instanceInfoEx.m_visibilityBits = modelDataEx.m_visibilityBits;
    instanceInfoEx.m_hashFrame = modelDataEx.m_hashFrame;

    auto transform = instanceInfoEx.m_Transform;
    uint32_t bottomLevelAccelStructId;

    if (instanceInfoEx.m_spPose != nullptr && instanceInfoEx.m_spPose->GetMatrixNum() > 1)
    {
        if (instanceInfoEx.m_poseVertexBuffer == nullptr)
        {
            uint32_t length = 0;
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                length += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position
            });

            if (length == 0)
                return;

            instanceInfoEx.m_poseVertexBuffer.Attach(new VertexBuffer(length));

            auto& message = s_messageSender.makeMessage<MsgCreateVertexBuffer>();
            message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
            message.allowUnorderedAccess = true;
            message.length = length;
            s_messageSender.endMessage();

            uint32_t offset = 0;
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                auto& copyMessage = s_messageSender.makeMessage<MsgCopyVertexBuffer>();
                copyMessage.dstVertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
                copyMessage.dstOffset = offset;
                copyMessage.srcVertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
                copyMessage.srcOffset = meshDataEx.m_VertexOffset;
                copyMessage.numBytes = meshDataEx.m_VertexNum * meshDataEx.m_VertexSize;
                s_messageSender.endMessage();

                offset += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position

            });
        }

        uint32_t geometryCount = 0;
        uint32_t nodeCount = 0;
        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            ++geometryCount;
            nodeCount += meshDataEx.m_NodeNum;
        });

        auto& message = s_messageSender.makeMessage<MsgComputePose>(
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix) +
            geometryCount * sizeof(MsgComputePose::GeometryDesc) +
            nodeCount * sizeof(uint32_t));

        message.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
        message.nodeCount = static_cast<uint8_t>(instanceInfoEx.m_spPose->GetMatrixNum());
        message.geometryCount = geometryCount;

        memcpy(message.data, instanceInfoEx.m_spPose->GetMatrixList(),
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        auto geometryDesc = reinterpret_cast<MsgComputePose::GeometryDesc*>(message.data +
            instanceInfoEx.m_spPose->GetMatrixNum() * sizeof(Hedgehog::Math::CMatrix));

        memset(geometryDesc, 0, geometryCount * sizeof(MsgComputePose::GeometryDesc));

        auto nodePalette = reinterpret_cast<uint32_t*>(geometryDesc + geometryCount);

        uint32_t vertexOffset = 0;
        traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
        {
            assert(meshDataEx.m_VertexOffset == 0);

            geometryDesc->vertexCount = meshDataEx.m_VertexNum;
            geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
            geometryDesc->vertexStride = static_cast<uint8_t>(meshDataEx.m_VertexSize);

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

            geometryDesc->nodeCount = static_cast<uint8_t>(meshDataEx.m_NodeNum);

            for (size_t i = 0; i < meshDataEx.m_NodeNum; i++)
                nodePalette[i] = meshDataEx.m_pNodeIndices[i] >= message.nodeCount ? 0 : static_cast<uint32_t>(meshDataEx.m_pNodeIndices[i]);

            if (shouldCheckForHash)
                MaterialData::create(*meshDataEx.m_spMaterial, true);

            if (meshDataEx.m_adjacency != nullptr)
            {
                auto& smoothNormalMsg = s_messageSender.makeMessage<MsgComputeSmoothNormal>();

                smoothNormalMsg.indexBufferId = meshDataEx.m_indices->getId();
                smoothNormalMsg.vertexStride = static_cast<uint8_t>(meshDataEx.m_VertexSize);
                smoothNormalMsg.vertexCount = meshDataEx.m_VertexNum;
                smoothNormalMsg.vertexOffset = vertexOffset;
                smoothNormalMsg.normalOffset = geometryDesc->normalOffset;
                smoothNormalMsg.vertexBufferId = instanceInfoEx.m_poseVertexBuffer->getId();
                smoothNormalMsg.adjacencyBufferId = meshDataEx.m_adjacency->getId();

                s_messageSender.endMessage();
            }

            vertexOffset += meshDataEx.m_VertexNum * (meshDataEx.m_VertexSize + 0xC); // Extra 12 bytes for previous position
            ++geometryDesc;
            nodePalette += meshDataEx.m_NodeNum;
        });

        s_messageSender.endMessage();

        if (instanceInfoEx.m_bottomLevelAccelStructId == NULL)
        {
            ::createBottomLevelAccelStruct(modelDataEx, instanceInfoEx.m_bottomLevelAccelStructId, 
                instanceInfoEx.m_poseVertexBuffer->getId());

            if (instanceInfoEx.m_bottomLevelAccelStructId == NULL)
                return;
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
        if (shouldCheckForHash)
        {
            traverseModelData(modelDataEx, [&](const MeshDataEx& meshDataEx, uint32_t)
            {
                MaterialData::create(*meshDataEx.m_spMaterial, true);
            });
        }

        if (instanceInfoEx.m_spPose != nullptr)
            transform = transform * (*instanceInfoEx.m_spPose->GetMatrixList());

        if (modelDataEx.m_bottomLevelAccelStructId == NULL)
        {
            ::createBottomLevelAccelStruct(modelDataEx, modelDataEx.m_bottomLevelAccelStructId, NULL);

            if (modelDataEx.m_bottomLevelAccelStructId == NULL)
                return;
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
        instanceInfoEx.m_instanceId = InstanceData::s_idAllocator.allocate();
        message.storePrevTransform = false;
    }
    else
    {
        message.storePrevTransform = true;
    }

    message.instanceId = instanceInfoEx.m_instanceId;
    message.bottomLevelAccelStructId = bottomLevelAccelStructId;
    message.isMirrored = false;

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

void ModelData::renderSky(Hedgehog::Mirage::CModelData& modelData)
{
    size_t geometryCount = 0;
    traverseModelData(modelData, [&](const MeshDataEx&, uint32_t) { ++geometryCount; });

    if (geometryCount == 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgRenderSky>(
        geometryCount * sizeof(MsgRenderSky::GeometryDesc));

    message.backgroundScale = *reinterpret_cast<const float*>(0x1A489EC);
    memset(message.data, 0, geometryCount * sizeof(MsgRenderSky::GeometryDesc));

    auto geometryDesc = reinterpret_cast<MsgRenderSky::GeometryDesc*>(message.data);

    static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");
    static Hedgehog::Base::CStringSymbol s_opacitySymbol("opacity");
    static Hedgehog::Base::CStringSymbol s_displacementSymbol("displacement");
    static Hedgehog::Base::CStringSymbol s_ambientSymbol("ambient");

    DX_PATCH::IDirect3DBaseTexture9* diffuseTexture = nullptr;

    traverseModelData(modelData, [&](const MeshDataEx& meshDataEx, uint32_t flags)
    {
        geometryDesc->flags = flags;
        geometryDesc->vertexBufferId = reinterpret_cast<const VertexBuffer*>(meshDataEx.m_pD3DVertexBuffer)->getId();
        geometryDesc->vertexStride = meshDataEx.m_VertexSize;
        geometryDesc->vertexCount = meshDataEx.m_VertexNum;
        geometryDesc->indexBufferId = reinterpret_cast<const IndexBuffer*>(meshDataEx.m_pD3DIndexBuffer)->getId();
        geometryDesc->indexCount = meshDataEx.m_IndexNum;
        geometryDesc->vertexDeclarationId = reinterpret_cast<const VertexDeclaration*>(meshDataEx.m_VertexDeclarationPtr.m_pD3DVertexDeclaration)->getId();
    
        if (meshDataEx.m_spMaterial != nullptr)
        {
            geometryDesc->isAdditive = meshDataEx.m_spMaterial->m_Additive;

            if (meshDataEx.m_spMaterial->m_spShaderListData != nullptr)
            {
                const char* bracket = strstr(meshDataEx.m_spMaterial->m_spShaderListData->m_TypeAndName.c_str(), "[");
                geometryDesc->enableVertexColor = bracket != nullptr && strstr(bracket, "v") != nullptr;
            }

            if (meshDataEx.m_spMaterial->m_spTexsetData != nullptr)
            {
                for (const auto& texture : meshDataEx.m_spMaterial->m_spTexsetData->m_TextureList)
                {
                    if (!texture->IsMadeOne() || texture->m_spPictureData == nullptr ||
                        texture->m_spPictureData->m_pD3DTexture == nullptr)
                    {
                        continue;
                    }
        
                    MsgCreateMaterial::Texture* textureDesc = nullptr;
        
                    if (texture->m_Type == s_diffuseSymbol)
                    {
                        textureDesc = &geometryDesc->diffuseTexture;
                        diffuseTexture = texture->m_spPictureData->m_pD3DTexture;
                    }
                    else if (texture->m_Type == s_opacitySymbol)
                    {
                        if (diffuseTexture == texture->m_spPictureData->m_pD3DTexture)
                            continue;

                        textureDesc = &geometryDesc->alphaTexture;
                    }
                    else if (texture->m_Type == s_displacementSymbol)
                    {
                        textureDesc = &geometryDesc->emissionTexture;
                    }
        
                    if (textureDesc != nullptr)
                    {
                        textureDesc->id = reinterpret_cast<const Texture*>(texture->m_spPictureData->m_pD3DTexture)->getId();
                        textureDesc->addressModeU = std::max(D3DTADDRESS_WRAP, texture->m_SamplerState.AddressU);
                        textureDesc->addressModeV = std::max(D3DTADDRESS_WRAP, texture->m_SamplerState.AddressV);
                        textureDesc->texCoordIndex = texture->m_TexcoordIndex;
                    }
                }
            }

            for (const auto& float4Param : meshDataEx.m_spMaterial->m_Float4Params)
            {
                if (float4Param->m_Name == s_diffuseSymbol)
                    memcpy(geometryDesc->diffuse, float4Param->m_spValue.get(), sizeof(geometryDesc->diffuse));

                if (float4Param->m_Name == s_ambientSymbol)
                    memcpy(geometryDesc->ambient, float4Param->m_spValue.get(), sizeof(geometryDesc->ambient));
            }
        }
        else
        {
            geometryDesc->isAdditive = false;
        }
    
        ++geometryDesc;
    });

    s_messageSender.endMessage();
}

void ModelData::init()
{
    WRITE_MEMORY(0x72FCDC, uint8_t, sizeof(TerrainModelDataEx));

    INSTALL_HOOK(TerrainModelDataConstructor);
    INSTALL_HOOK(TerrainModelDataDestructor);

    WRITE_MEMORY(0x4FA1FC, uint32_t, sizeof(ModelDataEx));
    WRITE_MEMORY(0xE993F1, uint32_t, sizeof(ModelDataEx));

    INSTALL_HOOK(ModelDataConstructor);
    INSTALL_HOOK(ModelDataDestructor);
}
