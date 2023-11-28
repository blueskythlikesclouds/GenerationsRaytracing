#include "MaterialData.h"

#include "FreeListAllocator.h"
#include "MaterialFlags.h"
#include "Message.h"
#include "MessageSender.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"

static std::unordered_set<Hedgehog::Mirage::CMaterialData*> s_materialsToCreate;
static Mutex s_matCreateMutex;

HOOK(MaterialDataEx*, __fastcall, MaterialDataConstructor, 0x704CA0, MaterialDataEx* This)
{
    const auto result = originalMaterialDataConstructor(This);

    This->m_materialId = NULL;
    new (&This->m_originalHighLightParamValue) boost::shared_ptr<float[]>();
    new (&This->m_raytracingHighLightParamValue) boost::shared_ptr<float[]>();
    This->m_materialHash = 0;
    This->m_hashFrame = 0;

    return result;
}

HOOK(void, __fastcall, MaterialDataDestructor, 0x704B80, MaterialDataEx* This)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.erase(This);
    s_matCreateMutex.unlock();

    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);
    This->m_originalHighLightParamValue.~shared_ptr();
    This->m_raytracingHighLightParamValue.~shared_ptr();

    originalMaterialDataDestructor(This);
}

static void createMaterial(MaterialDataEx& materialDataEx)
{
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();

    message.materialId = materialDataEx.m_materialId;
    message.shaderType = SHADER_TYPE_SYS_ERROR;
    message.flags = materialDataEx.m_Additive ? MATERIAL_FLAG_ADDITIVE : NULL;

    if (materialDataEx.m_DoubleSided)
        message.flags |= MATERIAL_FLAG_DOUBLE_SIDED;

    bool hasOpacityTexture = false;

    if (materialDataEx.m_spShaderListData != nullptr)
    {
        const auto shaderName = materialDataEx.m_spShaderListData->m_TypeAndName.c_str()
            + sizeof("Mirage.shader-list");

        const char* underscore = strstr(shaderName, "_");
        if (underscore != nullptr)
            hasOpacityTexture = strstr(underscore + 1, "a") != nullptr;

        for (const auto& [name, type] : s_shaderTypes)
        {
            if (strncmp(shaderName, name.data(), name.size()) == 0)
            {
                message.shaderType = type;
                break;
            }
        }
    }

    static Hedgehog::Base::CStringSymbol s_diffuseSymbol("diffuse");
    static Hedgehog::Base::CStringSymbol s_specularSymbol("specular");
    static Hedgehog::Base::CStringSymbol s_glossSymbol("gloss");
    static Hedgehog::Base::CStringSymbol s_normalSymbol("normal");
    static Hedgehog::Base::CStringSymbol s_reflectionSymbol("reflection");
    static Hedgehog::Base::CStringSymbol s_opacitySymbol("opacity");
    static Hedgehog::Base::CStringSymbol s_displacementSymbol("displacement");

    struct
    {
        Hedgehog::Base::CStringSymbol type;
        uint32_t offset;
        MsgCreateMaterial::Texture& dstTexture;
        DX_PATCH::IDirect3DBaseTexture9* dxpTexture;
    } textureDescs[] =
    {
        { s_diffuseSymbol, 0, message.diffuseTexture },
        { s_diffuseSymbol, 1, message.diffuseTexture2 },
        { s_specularSymbol, 0, message.specularTexture },
        { s_specularSymbol, 1, message.specularTexture2 },
        { s_glossSymbol, 0, message.glossTexture },
        { s_glossSymbol, 1, message.glossTexture2 },
        { s_normalSymbol, 0, message.normalTexture },
        { s_normalSymbol, 1, message.normalTexture2 },
        { s_reflectionSymbol, 0, message.reflectionTexture },
        { s_opacitySymbol, 0, message.opacityTexture },
        { s_displacementSymbol, 0, message.displacementTexture },
    };

    bool constTexCoord = true;

    for (auto& textureDesc : textureDescs)
    {
        textureDesc.dstTexture.id = 0;
        textureDesc.dstTexture.addressModeU = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.addressModeV = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.texCoordIndex = 0;

        if (materialDataEx.m_spTexsetData != nullptr)
        {
            uint32_t offset = 0;
            for (const auto& srcTexture : materialDataEx.m_spTexsetData->m_TextureList)
            {
                if (srcTexture->m_Type == textureDesc.type)
                {
                    if (textureDesc.offset != offset)
                    {
                        ++offset;
                    }
                    else
                    {
                        if (srcTexture->m_spPictureData != nullptr)
                        {
                            if (srcTexture->m_spPictureData->m_pD3DTexture == nullptr)
                            {
                                srcTexture->m_spPictureData->m_pD3DTexture =
                                    reinterpret_cast<DX_PATCH::IDirect3DBaseTexture9*>(new Texture(NULL, NULL, NULL));
                            }

                            // Skip materials that assign the diffuse texture as opacity
                            if (srcTexture->m_Type == s_opacitySymbol && (!hasOpacityTexture ||
                                srcTexture->m_spPictureData->m_pD3DTexture == textureDescs[0].dxpTexture))
                            {
                                continue;
                            }

                            textureDesc.dstTexture.id = reinterpret_cast<const Texture*>(
                                srcTexture->m_spPictureData->m_pD3DTexture)->getId();

                            textureDesc.dxpTexture = srcTexture->m_spPictureData->m_pD3DTexture;

                            if (srcTexture->m_Type == s_reflectionSymbol &&
                                srcTexture->m_spPictureData->m_TypeAndName == "Mirage.picture sph_st1_envmap_cube")
                            {
                                message.flags |= MATERIAL_FLAG_REFLECTION;
                            }
                        }
                        textureDesc.dstTexture.addressModeU = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressU);
                        textureDesc.dstTexture.addressModeV = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressV);
                        textureDesc.dstTexture.texCoordIndex = std::min<uint32_t>(srcTexture->m_TexcoordIndex, 3);

                        if (textureDesc.dstTexture.texCoordIndex != 0)
                            constTexCoord = false;

                        break;
                    }
                }
            }
        }
    }

    if (constTexCoord)
        message.flags |= MATERIAL_FLAG_CONST_TEX_COORD;

    static Hedgehog::Base::CStringSymbol s_texcoordOffsetSymbol("mrgTexcoordOffset");
    static Hedgehog::Base::CStringSymbol s_ambientSymbol("ambient");
    static Hedgehog::Base::CStringSymbol s_emissiveSymbol("emissive");
    static Hedgehog::Base::CStringSymbol s_powerGlossLevelSymbol("power_gloss_level");
    static Hedgehog::Base::CStringSymbol s_opacityReflectionRefractionSpectypeSymbol("opacity_reflection_refraction_spectype");
    static Hedgehog::Base::CStringSymbol s_luminanceRangeSymbol("mrgLuminanceRange");
    static Hedgehog::Base::CStringSymbol s_fresnelParamSymbol("mrgFresnelParam");
    static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightPositionRaytracingSymbol("g_SonicEyeHighLightPosition_Raytracing");
    static Hedgehog::Base::CStringSymbol s_sonicEyeHighLightColorSymbol("g_SonicEyeHighLightColor");
    static Hedgehog::Base::CStringSymbol s_sonicSkinFalloffParamSymbol("g_SonicSkinFalloffParam");
    static Hedgehog::Base::CStringSymbol s_chrEmissionParamSymbol("mrgChrEmissionParam");
    static Hedgehog::Base::CStringSymbol s_transColorMaskSymbol("g_TransColorMask");
    static Hedgehog::Base::CStringSymbol s_emissionParamSymbol("g_EmissionParam");
    static Hedgehog::Base::CStringSymbol s_offsetParamSymbol("g_OffsetParam");
    static Hedgehog::Base::CStringSymbol s_waterParamSymbol("g_WaterParam");

    struct
    {
        Hedgehog::Base::CStringSymbol name;
        float* destParam;
        uint32_t paramSize;
    } const parameterDescs[] =
    {
        { s_texcoordOffsetSymbol, message.texCoordOffsets, 8 },
        { s_diffuseSymbol, message.diffuse, 4 },
        { s_ambientSymbol, message.ambient, 3 },
        { s_specularSymbol, message.specular, 3 },
        { s_emissiveSymbol, message.emissive, 3 },
        { s_powerGlossLevelSymbol, message.powerGlossLevel, 3 },
        { s_opacityReflectionRefractionSpectypeSymbol, message.opacityReflectionRefractionSpectype, 1 },
        { s_luminanceRangeSymbol, message.luminanceRange, 1 },
        { s_fresnelParamSymbol, message.fresnelParam, 2 },
        { s_sonicEyeHighLightPositionRaytracingSymbol, message.sonicEyeHighLightPosition, 3 },
        { s_sonicEyeHighLightColorSymbol, message.sonicEyeHighLightColor, 3 },
        { s_sonicSkinFalloffParamSymbol, message.sonicSkinFalloffParam, 3 },
        { s_chrEmissionParamSymbol, message.chrEmissionParam, 4 },
        { s_transColorMaskSymbol, message.transColorMask, 3 },
        { s_emissionParamSymbol, message.emissionParam, 4 },
        { s_offsetParamSymbol, message.offsetParam, 2 },
        { s_waterParamSymbol, message.waterParam, 2 }
    };

    for (const auto& parameterDesc : parameterDescs)
    {
        memset(parameterDesc.destParam, 0, parameterDesc.paramSize * sizeof(float));

        for (const auto& float4Param : materialDataEx.m_Float4Params)
        {
            if (float4Param->m_Name == parameterDesc.name)
            {
                memcpy(parameterDesc.destParam, float4Param->m_spValue.get(), 
                    std::min(parameterDesc.paramSize, float4Param->m_ValueNum * 4) * sizeof(float));

                break;
            }
        }
    }

    s_messageSender.endMessage();
}

static void __fastcall materialDataSetMadeOne(MaterialDataEx* This)
{
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

    INSTALL_HOOK(MaterialAnimationEnv);
    INSTALL_HOOK(TexcoordAnimationEnv);
}

void MaterialData::postInit()
{
    // Better FxPipeline overrides those addresses, patch in PostInit to ensure priority
    WRITE_JUMP(0x74170C, materialDataSetMadeOne);
    WRITE_JUMP(0x741E00, materialDataSetMadeOne);
    WRITE_JUMP(0x7424DD, materialDataSetMadeOne);
    WRITE_JUMP(0x742BED, materialDataSetMadeOne);
}
