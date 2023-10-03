#include "PictureData.h"

#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

HOOK(Hedgehog::Mirage::CPictureData*, __fastcall, PictureDataCreate, 0x40CCE0, void* a1)
{
    const auto pictureData = originalPictureDataCreate(a1);

    // Create texture on creation time so we can access the ID immediately
    // when creating materials even if the texture is not loaded yet.
    pictureData->m_pD3DTexture = reinterpret_cast<DX_PATCH::IDirect3DBaseTexture9*>(new Texture(NULL, NULL, NULL));

    return pictureData;
}

HOOK(void, __cdecl, PictureDataMake, Hedgehog::Mirage::fpCPictureDataMake0,
     Hedgehog::Mirage::CPictureData* pictureData,
     uint8_t* data,
     size_t dataSize,
     Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (!pictureData->IsMadeOne())
    {
        const auto texture = reinterpret_cast<Texture*>(pictureData->m_pD3DTexture);
        assert(texture != nullptr);

        auto& message = s_messageSender.makeMessage<MsgMakeTexture>(dataSize);

        message.textureId = texture->getId();
#if _DEBUG
        strcpy(message.textureName, pictureData->m_TypeAndName.c_str() + 15);
#endif
        memcpy(message.data, data, dataSize);

        s_messageSender.endMessage();

        pictureData->m_Type = Hedgehog::Mirage::ePictureType_Texture;
        pictureData->SetMadeOne();
    }
}

void PictureData::init()
{
    INSTALL_HOOK(PictureDataCreate);
    INSTALL_HOOK(PictureDataMake);
}
