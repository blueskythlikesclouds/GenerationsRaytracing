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
    message.diffuseTexture.id = NULL;
    message.diffuseTexture.addressModeU = D3DTADDRESS_WRAP;
    message.diffuseTexture.addressModeV = D3DTADDRESS_WRAP;

    Hedgehog::Base::CStringSymbol diffuseSymbol("diffuse");

    for (const auto& textureData : This->m_spTexsetData->m_TextureList)
    {
        if (textureData->m_Type == diffuseSymbol && 
            textureData->m_spPictureData != nullptr &&
            textureData->m_spPictureData->m_pD3DTexture != nullptr)
        {
            message.diffuseTexture.id = reinterpret_cast<const Texture*>(
                textureData->m_spPictureData->m_pD3DTexture)->getId();

            message.diffuseTexture.addressModeU = textureData->m_SamplerState.AddressU;
            message.diffuseTexture.addressModeV = textureData->m_SamplerState.AddressV;

            break;
        }
    }

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