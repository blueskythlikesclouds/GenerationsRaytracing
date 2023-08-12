#include "PictureData.h"

#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

HOOK(void, __cdecl, CPictureDataMake, Hedgehog::Mirage::fpCPictureDataMake0,
     Hedgehog::Mirage::CPictureData* pictureData,
     uint8_t* data,
     size_t dataSize,
     Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    if (!pictureData->IsMadeOne())
    {
        Texture* texture = new Texture(NULL, NULL, NULL);

        auto& message = s_messageSender.makeParallelMessage<MsgMakeTexture>(dataSize);

        message.textureId = texture->getId();
        memcpy(message.data, data, dataSize);

        s_messageSender.endParallelMessage();

        pictureData->m_pD3DTexture = reinterpret_cast<DX_PATCH::IDirect3DBaseTexture9*>(texture);
        pictureData->m_Type = Hedgehog::Mirage::ePictureType_Texture;

        pictureData->SetMadeOne();
    }
}

void PictureData::init()
{
    INSTALL_HOOK(CPictureDataMake);
}
