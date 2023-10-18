#include "MaterialData.h"

#include "FreeListAllocator.h"
#include "MaterialFlags.h"
#include "Message.h"
#include "MessageSender.h"
#include "ShaderType.h"
#include "Texture.h"

static std::unordered_set<Hedgehog::Mirage::CMaterialData*> s_materialsToCreate;
static Mutex s_matCreateMutex;

HOOK(MaterialDataEx*, __fastcall, MaterialDataConstructor, 0x704CA0, MaterialDataEx* This)
{
    const auto result = originalMaterialDataConstructor(This);

    This->m_materialId = NULL;

    return result;
}

HOOK(void, __fastcall, MaterialDataDestructor, 0x704B80, MaterialDataEx* This)
{
    s_matCreateMutex.lock();
    s_materialsToCreate.erase(This);
    s_matCreateMutex.unlock();

    if (This->m_materialId != NULL)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
        message.resourceType = MsgReleaseRaytracingResource::ResourceType::Material;
        message.resourceId = This->m_materialId;
        s_messageSender.endMessage();

        MaterialData::s_idAllocator.free(This->m_materialId);
    }

    originalMaterialDataDestructor(This);
}

static void createMaterial(MaterialDataEx& materialDataEx)
{
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();

    message.materialId = materialDataEx.m_materialId;
    message.shaderType = SHADER_TYPE_COMMON;
    message.flags = materialDataEx.m_Additive ? MATERIAL_FLAG_ADDITIVE : NULL;

    if (materialDataEx.m_spShaderListData != nullptr)
    {
        const auto shaderName = materialDataEx.m_spShaderListData->m_TypeAndName.c_str()
            + sizeof("Mirage.shader-list");

        for (const auto& [name, type] : s_shaderTypes)
        {
            if (strncmp(shaderName, name.data(), name.size()) == 0)
            {
                message.shaderType = type;
                break;
            }
        }
    }

    const Hedgehog::Base::CStringSymbol diffuseSymbol("diffuse");
    const Hedgehog::Base::CStringSymbol specularSymbol("specular");
    const Hedgehog::Base::CStringSymbol glossSymbol("gloss");
    const Hedgehog::Base::CStringSymbol normalSymbol("normal");
    const Hedgehog::Base::CStringSymbol reflectionSymbol("reflection");
    const Hedgehog::Base::CStringSymbol opacitySymbol("opacity");
    const Hedgehog::Base::CStringSymbol displacementSymbol("displacement");

    struct
    {
        Hedgehog::Base::CStringSymbol type;
        uint32_t offset;
        MsgCreateMaterial::Texture& dstTexture;
        DX_PATCH::IDirect3DBaseTexture9* dxpTexture;
    } textureDescs[] =
    {
        { diffuseSymbol, 0, message.diffuseTexture },
        { diffuseSymbol, 1, message.diffuseTexture2 },
        { specularSymbol, 0, message.specularTexture },
        { specularSymbol, 1, message.specularTexture2 },
        { glossSymbol, 0, message.glossTexture },
        { glossSymbol, 1, message.glossTexture2 },
        { normalSymbol, 0, message.normalTexture },
        { normalSymbol, 1, message.normalTexture2 },
        { reflectionSymbol, 0, message.reflectionTexture },
        { opacitySymbol, 0, message.opacityTexture },
        { displacementSymbol, 0, message.displacementTexture },
        { displacementSymbol, 1, message.displacementTexture1 },
        { displacementSymbol, 2, message.displacementTexture2 },
    };

    for (auto& textureDesc : textureDescs)
    {
        textureDesc.dstTexture.id = 0;
        textureDesc.dstTexture.addressModeU = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.addressModeV = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.texCoordIndex = 0;

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
                        if (srcTexture->m_Type == opacitySymbol && 
                            srcTexture->m_spPictureData->m_pD3DTexture == textureDescs[0].dxpTexture)
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

    struct
    {
        Hedgehog::Base::CStringSymbol name;
        float* destParam;
        uint32_t paramSize;
    } const parameterDescs[] =
    {
        { "mrgTexcoordOffset", message.texCoordOffsets, 8 },
        { diffuseSymbol, message.diffuse, 4 },
        { "ambient", message.ambient, 4 },
        { specularSymbol, message.specular, 4 },
        { "emissive", message.emissive, 4 },
        { "power_gloss_level", message.powerGlossLevel, 4 },
        { "opacity_reflection_refraction_spectype", message.opacityReflectionRefractionSpectype, 4 },
        { "mrgLuminanceRange", message.luminanceRange, 4 },
        { "mrgFresnelParam", message.fresnelParam, 4 },
        { "g_SonicEyeHighLightPosition", message.sonicEyeHighLightPosition, 4 },
        { "g_SonicEyeHighLightColor", message.sonicEyeHighLightColor, 4 },
        { "g_SonicSkinFalloffParam", message.sonicSkinFalloffParam, 4 },
        { "mrgChrEmissionParam", message.chrEmissionParam, 4 },
        //{ "g_CloakParam", message.cloakParam, 4 },
        //{ "mrgDistortionParam", message.distortionParam, 4 },
        //{ "mrgGlassRefractionParam", message.glassRefractionParam, 4 },
        //{ "g_IceParam", message.iceParam, 4 },
        { "g_EmissionParam", message.emissionParam, 4 },
        { "g_OffsetParam", message.offsetParam, 4 },
        { "g_HeightParam", message.heightParam, 4 },
        { "g_WaterParam", message.waterParam, 4 }
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
        {
            materialDataEx.m_materialId = s_idAllocator.allocate();
            createMaterial(materialDataEx);
        }
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

    WRITE_JUMP(0x74170C, materialDataSetMadeOne);
    WRITE_JUMP(0x741E00, materialDataSetMadeOne);
    WRITE_JUMP(0x7424DD, materialDataSetMadeOne);
    WRITE_JUMP(0x742BED, materialDataSetMadeOne);

    INSTALL_HOOK(SampleTexcoordAnimation);
    INSTALL_HOOK(SampleMaterialAnimation);
}