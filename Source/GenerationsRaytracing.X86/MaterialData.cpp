#include "MaterialData.h"

#include "FreeListAllocator.h"
#include "MaterialFlags.h"
#include "Message.h"
#include "MessageSender.h"
#include "RaytracingUtil.h"
#include "ShaderType.h"
#include "Texture.h"

static std::unordered_set<Hedgehog::Mirage::CMaterialData*> s_materialsToCreate;
static Mutex s_matCreateMutex;

HOOK(MaterialDataEx*, __fastcall, MaterialDataConstructor, 0x704CA0, MaterialDataEx* This)
{
    const auto result = originalMaterialDataConstructor(This);

    This->m_materialId = NULL;
    new (&This->m_highLightParamValue) boost::shared_ptr<float[]>();

    return result;
}

HOOK(void, __fastcall, MaterialDataDestructor, 0x704B80, MaterialDataEx* This)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.erase(This);
    s_matCreateMutex.unlock();

    RaytracingUtil::releaseResource(RaytracingResourceType::Material, This->m_materialId);
    This->m_highLightParamValue.~shared_ptr();

    originalMaterialDataDestructor(This);
}

static void createMaterial(MaterialDataEx& materialDataEx)
{
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();

    message.materialId = materialDataEx.m_materialId;
    message.shaderType = SHADER_TYPE_COMMON;
    message.flags = materialDataEx.m_Additive ? MATERIAL_FLAG_ADDITIVE : NULL;

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
        { s_displacementSymbol, 1, message.displacementTexture1 },
        { s_displacementSymbol, 2, message.displacementTexture2 },
    };

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
                        }
                        textureDesc.dstTexture.addressModeU = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressU);
                        textureDesc.dstTexture.addressModeV = std::max(D3DTADDRESS_WRAP, srcTexture->m_SamplerState.AddressV);
                        textureDesc.dstTexture.texCoordIndex = std::min<uint32_t>(srcTexture->m_TexcoordIndex, 3);

                        break;
                    }
                }
            }
        }
    }

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
    static Hedgehog::Base::CStringSymbol s_cloakParamSymbol("g_CloakParam");
    static Hedgehog::Base::CStringSymbol s_transColorMaskSymbol("g_TransColorMask");
    static Hedgehog::Base::CStringSymbol s_distortionParamSymbol("mrgDistortionParam");
    static Hedgehog::Base::CStringSymbol s_glassRefractionParamSymbol("mrgGlassRefractionParam");
    static Hedgehog::Base::CStringSymbol s_iceParamSymbol("g_IceParam");
    static Hedgehog::Base::CStringSymbol s_emissionParamSymbol("g_EmissionParam");
    static Hedgehog::Base::CStringSymbol s_offsetParamSymbol("g_OffsetParam");
    static Hedgehog::Base::CStringSymbol s_heightParamSymbol("g_HeightParam");
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
        { s_ambientSymbol, message.ambient, 4 },
        { s_specularSymbol, message.specular, 4 },
        { s_emissiveSymbol, message.emissive, 4 },
        { s_powerGlossLevelSymbol, message.powerGlossLevel, 4 },
        { s_opacityReflectionRefractionSpectypeSymbol, message.opacityReflectionRefractionSpectype, 4 },
        { s_luminanceRangeSymbol, message.luminanceRange, 4 },
        { s_fresnelParamSymbol, message.fresnelParam, 4 },
        { s_sonicEyeHighLightPositionRaytracingSymbol, message.sonicEyeHighLightPosition, 4 },
        { s_sonicEyeHighLightColorSymbol, message.sonicEyeHighLightColor, 4 },
        { s_sonicSkinFalloffParamSymbol, message.sonicSkinFalloffParam, 4 },
        { s_chrEmissionParamSymbol, message.chrEmissionParam, 4 },
        //{ s_cloakParamSymbol, message.cloakParam, 4 },
        { s_transColorMaskSymbol, message.transColorMask, 4 },
        //{ s_distortionParamSymbol, message.distortionParam, 4 },
        //{ s_glassRefractionParamSymbol, message.glassRefractionParam, 4 },
        //{ s_iceParamSymbol, message.iceParam, 4 },
        { s_emissionParamSymbol, message.emissionParam, 4 },
        { s_offsetParamSymbol, message.offsetParam, 4 },
        { s_heightParamSymbol, message.heightParam, 4 },
        { s_waterParamSymbol, message.waterParam, 4 }
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

HOOK(void, __cdecl, SampleTexcoordAnimation, 0x757E50, Hedgehog::Mirage::CMaterialData* materialData, void* a2, float a3, void* a4)
{
    originalSampleTexcoordAnimation(materialData, a2, a3, a4);

    auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(materialData);
    if (materialDataEx.m_materialId != NULL)
        createMaterial(materialDataEx);
}

HOOK(void, __cdecl, SampleMaterialAnimation, 0x757380, Hedgehog::Mirage::CMaterialData* materialData, void* a2, float a3)
{
    originalSampleMaterialAnimation(materialData, a2, a3);

    auto& materialDataEx = *reinterpret_cast<MaterialDataEx*>(materialData);
    if (materialDataEx.m_materialId != NULL)
        createMaterial(materialDataEx);
}

bool MaterialData::create(Hedgehog::Mirage::CMaterialData& materialData)
{
    if (materialData.IsMadeAll())
    {
        auto& materialDataEx = reinterpret_cast<MaterialDataEx&>(materialData);

        if (materialDataEx.m_materialId == NULL)
            materialDataEx.m_materialId = s_idAllocator.allocate();

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
        if (create(**it))
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

    INSTALL_HOOK(SampleTexcoordAnimation);
    INSTALL_HOOK(SampleMaterialAnimation);
}

void MaterialData::postInit()
{
    // Better FxPipeline overrides those addresses, patch in PostInit to ensure priority
    WRITE_JUMP(0x74170C, materialDataSetMadeOne);
    WRITE_JUMP(0x741E00, materialDataSetMadeOne);
    WRITE_JUMP(0x7424DD, materialDataSetMadeOne);
    WRITE_JUMP(0x742BED, materialDataSetMadeOne);
}
