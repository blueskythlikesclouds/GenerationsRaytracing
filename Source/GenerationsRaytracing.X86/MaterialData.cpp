#include "MaterialData.h"

#include "FreeListAllocator.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

static FreeListAllocator s_freeListAllocator;

HOOK(MaterialDataEx*, __fastcall, MaterialDataConstructor, 0x704CA0, MaterialDataEx* This)
{
    const auto result = originalMaterialDataConstructor(This);

    This->m_materialId = s_freeListAllocator.allocate();

    return result;
}

HOOK(void, __fastcall, MaterialDataDestructor, 0x704B80, MaterialDataEx* This)
{
    auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();

    message.resourceType = MsgReleaseRaytracingResource::ResourceType::Material;
    message.resourceId = This->m_materialId;

    s_messageSender.endMessage();

    s_freeListAllocator.free(This->m_materialId);

    originalMaterialDataDestructor(This);
}

static void __fastcall materialDataSetMadeOne(MaterialDataEx* This)
{
    auto& message = s_messageSender.makeMessage<MsgCreateMaterial>();

    message.materialId = This->m_materialId;

    const Hedgehog::Base::CStringSymbol diffuseSymbol("diffuse");
    const Hedgehog::Base::CStringSymbol specularSymbol("specular");
    const Hedgehog::Base::CStringSymbol glossSymbol("gloss");
    const Hedgehog::Base::CStringSymbol normalSymbol("normal");
    const Hedgehog::Base::CStringSymbol displacementSymbol("displacement");
    const Hedgehog::Base::CStringSymbol reflectionSymbol("reflection");

    struct
    {
        Hedgehog::Base::CStringSymbol type;
        uint32_t offset;
        MsgCreateMaterial::Texture& dstTexture;
    } const textureDescs[] = 
    {
        { diffuseSymbol, 0, message.diffuseTexture },
        { specularSymbol, 0, message.specularTexture },
        { glossSymbol, 0, message.specularPowerTexture },
        { normalSymbol, 0, message.normalTexture },
        { displacementSymbol, 0, message.emissionTexture },
        { diffuseSymbol, 1, message.diffuseBlendTexture },
        { glossSymbol, 1, message.powerBlendTexture },
        { normalSymbol, 1, message.normalBlendTexture },
        { reflectionSymbol, 0, message.environmentTexture },
    };

    bool hasSpecular = false;
    bool hasGloss = false;

    for (const auto& textureDesc : textureDescs)
    {
        textureDesc.dstTexture.id = 0;
        textureDesc.dstTexture.addressModeU = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.addressModeV = D3DTADDRESS_WRAP;
        textureDesc.dstTexture.texCoordIndex = 0;

        uint32_t offset = 0;
        for (const auto& srcTexture : This->m_spTexsetData->m_TextureList)
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

                        textureDesc.dstTexture.id = reinterpret_cast<const Texture*>(
                            srcTexture->m_spPictureData->m_pD3DTexture)->getId();
                    }
                    textureDesc.dstTexture.addressModeU = srcTexture->m_SamplerState.AddressU;
                    textureDesc.dstTexture.addressModeV = srcTexture->m_SamplerState.AddressV;
                    textureDesc.dstTexture.texCoordIndex = srcTexture->m_TexcoordIndex;

                    if (srcTexture->m_Type == specularSymbol) hasSpecular = true;
                    if (srcTexture->m_Type == glossSymbol) hasGloss = true;

                    break;
                }
            }
        }
    }

    float diffuseColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    float specularColor[3]{ 1.0f, 1.0f, 1.0f };
    float specularPower = 0.0f;
    float falloffParam[3]{ 0.0f, 0.0f, 0.0f };

    const Hedgehog::Base::CStringSymbol opacityReflectionRefractionSpectypeSymbol("opacity_reflection_refraction_spectype");
    const Hedgehog::Base::CStringSymbol powerGlossLevelSymbol("power_gloss_level");
    const Hedgehog::Base::CStringSymbol falloffParamSymbol("g_SonicSkinFalloffParam");

    for (const auto& float4Param : This->m_Float4Params)
    {
        if (float4Param->m_Name == diffuseSymbol)
        {
            diffuseColor[0] = float4Param->m_spValue[0];
            diffuseColor[1] = float4Param->m_spValue[1];
            diffuseColor[2] = float4Param->m_spValue[2];
        }
        else if (float4Param->m_Name == specularSymbol)
        {
            specularColor[0] *= float4Param->m_spValue[0];
            specularColor[1] *= float4Param->m_spValue[1];
            specularColor[2] *= float4Param->m_spValue[2];
        }
        else if (float4Param->m_Name == opacityReflectionRefractionSpectypeSymbol)
        {
            diffuseColor[3] = float4Param->m_spValue[0];
        }
        else if (float4Param->m_Name == powerGlossLevelSymbol)
        {
            const float level = float4Param->m_spValue[2] * 5.0f;

            specularColor[0] *= level;
            specularColor[1] *= level;
            specularColor[2] *= level;

            specularPower = float4Param->m_spValue[1] * 500.0f;
        }
        else if (float4Param->m_Name == falloffParamSymbol)
        {
            falloffParam[0] = float4Param->m_spValue[0];
            falloffParam[1] = float4Param->m_spValue[1];
            falloffParam[2] = float4Param->m_spValue[2];
        }
    }
    if (!hasSpecular && !hasGloss)
    {
        specularColor[0] = 0.0f;
        specularColor[1] = 0.0f;
        specularColor[2] = 0.0f;
    }

    memcpy(message.diffuseColor, diffuseColor, sizeof(message.diffuseColor));
    memcpy(message.specularColor, specularColor, sizeof(message.specularColor));
    message.specularPower = specularPower;
    memcpy(message.falloffParam, falloffParam, sizeof(message.falloffParam));

    s_messageSender.endMessage();

    This->SetMadeOne();
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

    // TODO: Add support for other versions somehow
    WRITE_JUMP(0x742BED, materialDataSetMadeOne);
}