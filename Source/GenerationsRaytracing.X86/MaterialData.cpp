#include "MaterialData.h"

#include "FreeListAllocator.h"
#include "MaterialFlags.h"
#include "Message.h"
#include "MessageSender.h"
#include "RaytracingRendering.h"
#include "RaytracingShader.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"

static std::unordered_set<Hedgehog::Mirage::CMaterialData*> s_materialsToCreate;
static Mutex s_matCreateMutex;

HOOK(MaterialDataEx*, __fastcall, MaterialDataConstructor, 0x704CA0, MaterialDataEx* This)
{
    const auto result = originalMaterialDataConstructor(This);

    This->m_materialId = NULL;
    This->m_materialHash = 0;
    This->m_hashFrame = 0;
    new (&This->m_fhlMaterial) boost::shared_ptr<Hedgehog::Mirage::CMaterialData>();

    return result;
}

HOOK(void, __fastcall, MaterialDataDestructor, 0x704B80, MaterialDataEx* This)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.erase(This);
    s_matCreateMutex.unlock();

    This->m_fhlMaterial.~shared_ptr();
    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);

    originalMaterialDataDestructor(This);
}

HOOK(void, __cdecl, MaterialDataMake, 0x733E60,
    const Hedgehog::Base::CSharedString& name,
    const void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    const char* fhlSuffix = strstr(name.c_str(), "_fhl");

    if (fhlSuffix == (name.data() + name.size() - 4) && data != nullptr)
    {
        Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(database.get());

        const auto materialData = wrapper.GetMaterialData(name.substr(0, name.size() - 4));
        if (materialData != nullptr)
        {
            auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(materialData.get());
            materialDataEx.m_fhlMaterial = wrapper.GetMaterialData(name);
        }
    }

    originalMaterialDataMake(name, data, dataSize, database, renderingInfrastructure);
}

static void createMaterial(MaterialDataEx& materialDataEx)
{
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();

    message.materialId = materialDataEx.m_materialId;
    message.shaderType = SHADER_TYPE_SYS_ERROR;
    message.flags = materialDataEx.m_Additive ? MATERIAL_FLAG_ADDITIVE : NULL;

    if (materialDataEx.m_DoubleSided)
        message.flags |= MATERIAL_FLAG_DOUBLE_SIDED;

    auto shader = &s_shader_SYS_ERROR;
    bool hasOpacityTexture = false;

    if (materialDataEx.m_spShaderListData != nullptr)
    {
        const auto shaderName = materialDataEx.m_spShaderListData->m_TypeAndName.c_str()
            + sizeof("Mirage.shader-list");

        const char* underscore = strstr(shaderName, "_");
        if (underscore != nullptr)
            hasOpacityTexture = strstr(underscore + 1, "a") != nullptr;

        for (const auto& [name, alsoShader] : s_shaders)
        {
            if (strncmp(shaderName, name.data(), name.size()) == 0)
            {
                shader = alsoShader;
                break;
            }
        }

        message.shaderType = shader->type;

        if (strstr(shaderName, "SoftEdge") != nullptr)
            message.flags |= MATERIAL_FLAG_SOFT_EDGE;

        if (strstr(shaderName, "noGIs") != nullptr || strstr(shaderName, "BlbBlend") != nullptr)
            message.flags |= MATERIAL_FLAG_NO_SHADOW;

        if (strstr(shaderName, "Blb") != nullptr)
            message.flags |= MATERIAL_FLAG_VIEW_Z_ALPHA_FADE;
    }

    if (materialDataEx.m_spTexsetData != nullptr)
    {
        static Hedgehog::Base::CStringSymbol s_reflectionSymbol("reflection");
        for (const auto& texture : materialDataEx.m_spTexsetData->m_TextureList)
        {
            if (texture->m_Type == s_reflectionSymbol && (
                texture->m_spPictureData->m_TypeAndName == "Mirage.picture sph_st1_envmap_cube" ||
                texture->m_spPictureData->m_TypeAndName == "Mirage.picture ghz_water_km1_puddle_cubemap" ||
                texture->m_spPictureData->m_TypeAndName == "Mirage.picture cpz_uni_tn1_cubemap" ||
                texture->m_spPictureData->m_TypeAndName == "Mirage.picture csc_metal_ty1_envmap_cube01"))
            {
                message.flags |= MATERIAL_FLAG_REFLECTION;
                break;
            }
        }
    }

    bool constTexCoord = true;
    message.textureNum = shader->textureNum;

    for (size_t i = 0; i < shader->textureNum; i++)
    {
        const auto texture = shader->textures[i];
        auto& dstTexture = message.textures[i];

        dstTexture.id = 0;
        dstTexture.addressModeU = D3DTADDRESS_WRAP;
        dstTexture.addressModeV = D3DTADDRESS_WRAP;
        dstTexture.texCoordIndex = 0;

        if ((message.shaderType == SHADER_TYPE_COMMON ||
            message.shaderType == SHADER_TYPE_IGNORE_LIGHT) &&
            texture->isOpacity && !hasOpacityTexture)
        {
            continue;
        }

        if (texture->nameSymbol == Hedgehog::Base::CStringSymbol{})
            texture->nameSymbol = texture->name;

        if (materialDataEx.m_spTexsetData != nullptr)
        {
            uint32_t offset = 0;
            for (const auto& srcTexture : materialDataEx.m_spTexsetData->m_TextureList)
            {
                if (srcTexture->m_Type == texture->nameSymbol)
                {
                    if (texture->index != offset)
                    {
                        ++offset;
                    }
                    else
                    {
                        if (srcTexture->m_spPictureData != nullptr &&
                            srcTexture->m_spPictureData->m_pD3DTexture != nullptr)
                        {
                            dstTexture.id = reinterpret_cast<const Texture*>(
                                srcTexture->m_spPictureData->m_pD3DTexture)->getId();
                        }
                        dstTexture.addressModeU = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressU);
                        dstTexture.addressModeV = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressV);
                        dstTexture.texCoordIndex = std::min<uint32_t>(srcTexture->m_TexcoordIndex, 3);

                        if (dstTexture.texCoordIndex != 0)
                            constTexCoord = false;

                        break;
                    }
                }
            }
        }
    }

    if (constTexCoord)
        message.flags |= MATERIAL_FLAG_CONST_TEX_COORD;

    static Hedgehog::Base::CStringSymbol s_texCoordOffsetSymbol("mrgTexcoordOffset");
    memset(message.texCoordOffsets, 0, sizeof(message.texCoordOffsets));

    for (const auto& float4Param : materialDataEx.m_Float4Params)
    {
        if (float4Param->m_Name == s_texCoordOffsetSymbol)
        {
            memcpy(message.texCoordOffsets, float4Param->m_spValue.get(),
                std::min(float4Param->m_ValueNum * 4, 8u) * sizeof(float));

            break;
        }
    }

    message.parameterNum = 0;
    float* destParam = message.parameters;

    for (size_t i = 0; i < shader->parameterNum; i++)
    {
        const auto parameter = shader->parameters[i];
        memcpy(destParam, &((&parameter->x)[parameter->index]), parameter->size * sizeof(float));

        if (parameter->nameSymbol == Hedgehog::Base::CStringSymbol{})
            parameter->nameSymbol = parameter->name;

        bool found = false;

        for (const auto& float4Param : materialDataEx.m_Float4Params)
        {
            if (float4Param->m_Name == parameter->nameSymbol)
            {
                memcpy(destParam, &float4Param->m_spValue[parameter->index], 
                    std::min(parameter->size, float4Param->m_ValueNum * 4 - parameter->index) * sizeof(float));

                found = true;
                break;
            }
        }

        if (!found)
        {
            auto float4Param = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();
            float4Param->m_Name = parameter->nameSymbol;
            float4Param->m_ValueNum = 1;
            float4Param->m_spValue = boost::make_shared<float[]>(4);
            float4Param->m_spValue[0] = parameter->x;
            float4Param->m_spValue[1] = parameter->y;
            float4Param->m_spValue[2] = parameter->z;
            float4Param->m_spValue[3] = parameter->w;
            materialDataEx.m_Float4Params.push_back(float4Param);
        }

        message.parameterNum += parameter->size;
        destParam += parameter->size;
    }

    assert(message.textureNum <= _countof(message.textures));
    assert(message.parameterNum <= _countof(message.parameters));

    s_messageSender.endMessage();
}

static void __fastcall materialDataSetMadeOne(MaterialDataEx* This)
{
    if (This->m_fhlMaterial != nullptr)
    {
        static Hedgehog::Base::CStringSymbol s_texcoordOffsetSymbol("mrgTexcoordOffset");

        boost::shared_ptr<float[]> value;
        for (const auto& float4Param : This->m_Float4Params)
        {
            if (float4Param->m_Name == s_texcoordOffsetSymbol && float4Param->m_ValueNum == 2)
            {
                value = float4Param->m_spValue;
                break;
            }
        }
        if (value == nullptr)
        {
            value = boost::make_shared<float[]>(8, 0.0f);

            const auto float4Param = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();
            float4Param->m_Name = s_texcoordOffsetSymbol;
            float4Param->m_ValueNum = 2;
            float4Param->m_spValue = value;

            This->m_Float4Params.push_back(float4Param);
        }

        for (const auto& float4Param : This->m_fhlMaterial->m_Float4Params)
        {
            if (float4Param->m_Name == s_texcoordOffsetSymbol && float4Param->m_ValueNum == 2)
            {
                float4Param->m_spValue = std::move(value);
                break;
            }
        }
        if (value != nullptr)
        {
            const auto float4Param = boost::make_shared<Hedgehog::Mirage::CParameterFloat4Element>();
            float4Param->m_Name = s_texcoordOffsetSymbol;
            float4Param->m_ValueNum = 2;
            float4Param->m_spValue = value;

            This->m_fhlMaterial->m_Float4Params.push_back(float4Param);
        }
    }

    LockGuard lock(s_matCreateMutex);
    s_materialsToCreate.emplace(This);

    This->SetMadeOne();
}

HOOK(void, __fastcall, MaterialAnimationEnv, 0x57C1C0, uintptr_t This, uintptr_t Edx, float deltaTime)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.emplace(*reinterpret_cast<Hedgehog::Mirage::CMaterialData**>(This + 12));
    s_matCreateMutex.unlock();

    originalMaterialAnimationEnv(This, Edx, deltaTime);
}

HOOK(void, __fastcall, TexcoordAnimationEnv, 0x57C380, uintptr_t This, uintptr_t Edx, float deltaTime)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.emplace(*reinterpret_cast<Hedgehog::Mirage::CMaterialData**>(This + 12));
    s_matCreateMutex.unlock();

    originalTexcoordAnimationEnv(This, Edx, deltaTime);
}

static FUNCTION_PTR(void, __thiscall, cloneMaterial, 0x704CE0,
    Hedgehog::Mirage::CMaterialData* This, Hedgehog::Mirage::CMaterialData* rValue);

HOOK(void, __fastcall, SingleElementEmplaceMaterialMap, 0x701CC0,
    Hedgehog::Mirage::CSingleElement* This,
    void* _,
    Hedgehog::Mirage::CMaterialData* key,
    const boost::shared_ptr<Hedgehog::Mirage::CMaterialData>& value)
{
    const auto keyEx = reinterpret_cast<MaterialDataEx*>(key);
    if (keyEx->m_fhlMaterial != nullptr)
    {
        const auto materialClone = static_cast<Hedgehog::Mirage::CMaterialData*>(__HH_ALLOC(sizeof(MaterialDataEx)));
        Hedgehog::Mirage::fpCMaterialDataCtor(materialClone);
        cloneMaterial(materialClone, keyEx->m_fhlMaterial.get());
        This->m_MaterialMap.emplace(keyEx->m_fhlMaterial.get(), boost::shared_ptr<Hedgehog::Mirage::CMaterialData>(materialClone));
    }

    return originalSingleElementEmplaceMaterialMap(This, _, key, value);
}

bool MaterialData::create(Hedgehog::Mirage::CMaterialData& materialData, bool checkForHash)
{
    if (materialData.IsMadeAll())
    {
        auto& materialDataEx = reinterpret_cast<MaterialDataEx&>(materialData);

        bool shouldCreate = true;

        if (materialDataEx.m_materialId == NULL)
        {
            materialDataEx.m_materialId = s_idAllocator.allocate();
        }
        else if (checkForHash)
        {
            if (materialDataEx.m_hashFrame != RaytracingRendering::s_frame)
            {
                XXH32_state_t state;
                XXH32_reset(&state, 0);

                if (materialDataEx.m_spShaderListData != nullptr)
                    XXH32_update(&state, &materialDataEx.m_spShaderListData, sizeof(materialDataEx.m_spShaderListData));

                for (const auto& float4Param : materialData.m_Float4Params)
                    XXH32_update(&state, float4Param->m_spValue.get(), float4Param->m_ValueNum * sizeof(float[4]));

                if (materialData.m_spTexsetData != nullptr)
                {
                    for (const auto& textureData : materialData.m_spTexsetData->m_TextureList)
                    {
                        if (textureData->m_spPictureData != nullptr)
                            XXH32_update(&state, &textureData->m_spPictureData->m_pD3DTexture, sizeof(textureData->m_spPictureData->m_pD3DTexture));
                    }
                }

                const XXH32_hash_t materialHash = XXH32_digest(&state);

                shouldCreate = materialDataEx.m_materialHash != materialHash;

                materialDataEx.m_materialHash = materialHash;
                materialDataEx.m_hashFrame = RaytracingRendering::s_frame;
            }
            else
            {
                shouldCreate = false;
            }
        }

        if (shouldCreate)
            createMaterial(materialDataEx);

        return true;
    }
    return false;
}

void MaterialData::createPendingMaterials()
{
    LockGuard lock(s_matCreateMutex);

    for (auto it = s_materialsToCreate.begin(); it != s_materialsToCreate.end();)
    {
        if (create(**it, false))
            it = s_materialsToCreate.erase(it);
        else
            ++it;
    }
}

void MaterialData::init()
{
    WRITE_MEMORY(0x6C6301, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0x72FB5C, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xDD8B46, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xDD8BAB, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xE151E0, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xE15241, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xE704DA, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xE7053B, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xE99B29, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0xEC04F0, uint8_t, sizeof(MaterialDataEx));
    WRITE_MEMORY(0x11F900B, uint8_t, sizeof(MaterialDataEx));

    INSTALL_HOOK(MaterialDataConstructor);
    INSTALL_HOOK(MaterialDataDestructor);

    INSTALL_HOOK(MaterialDataMake);

    INSTALL_HOOK(MaterialAnimationEnv);
    INSTALL_HOOK(TexcoordAnimationEnv);

    INSTALL_HOOK(SingleElementEmplaceMaterialMap);
}

void MaterialData::postInit()
{
    // Better FxPipeline overrides those addresses, patch in PostInit to ensure priority
    WRITE_JUMP(0x74170C, materialDataSetMadeOne);
    WRITE_JUMP(0x741E00, materialDataSetMadeOne);
    WRITE_JUMP(0x7424DD, materialDataSetMadeOne);
    WRITE_JUMP(0x742BED, materialDataSetMadeOne);
}
