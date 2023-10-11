#include "PictureData.h"

#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

HOOK(void, __cdecl, PictureDataMake, Hedgehog::Mirage::fpCPictureDataMake0,
     Hedgehog::Mirage::CPictureData* pictureData,
     uint8_t* data,
     size_t dataSize,
     Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (!pictureData->IsMadeOne())
    {
        Texture* texture;

        if (pictureData->m_pD3DTexture != nullptr)
        {
            texture = reinterpret_cast<Texture*>(pictureData->m_pD3DTexture);
        }
        else
        {
            texture = new Texture(NULL, NULL, NULL);
            pictureData->m_pD3DTexture = reinterpret_cast<DX_PATCH::IDirect3DBaseTexture9*>(texture);
        }

        if (*reinterpret_cast<uint32_t*>(data) == MAKEFOURCC('D', 'D', 'S', ' '))
        {
            texture->setResolution(
                *reinterpret_cast<uint32_t*>(data + 16),
                *reinterpret_cast<uint32_t*>(data + 12));

            auto& message = s_messageSender.makeMessage<MsgMakeTexture>(dataSize);

            message.textureId = texture->getId();
#if _DEBUG
            strcpy(message.textureName, pictureData->m_TypeAndName.c_str() + 15);
#endif
            memcpy(message.data, data, dataSize);

            s_messageSender.endMessage();
        }

        pictureData->m_Type = Hedgehog::Mirage::ePictureType_Texture;
        pictureData->SetMadeOne();
    }
}

void PictureData::init()
{
    INSTALL_HOOK(PictureDataMake);
}
