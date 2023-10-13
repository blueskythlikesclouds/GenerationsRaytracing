#include "BaseTexture.h"

#include "FreeListAllocator.h"
#include "Message.h"
#include "MessageSender.h"

static FreeListAllocator s_idAllocator;

BaseTexture::BaseTexture(uint32_t levelCount)
{
    m_id = s_idAllocator.allocate();
    m_levelCount = levelCount;
}

BaseTexture::~BaseTexture()
{
    auto& message = s_messageSender.makeMessage<MsgReleaseResource>();

    message.resourceType = MsgReleaseResource::ResourceType::Texture;
    message.resourceId = m_id;

    s_messageSender.endMessage();

    s_idAllocator.free(m_id);
}

uint32_t BaseTexture::getId() const
{
    return m_id;
}

FUNCTION_STUB(DWORD, NULL, BaseTexture::SetLOD, DWORD LODNew)

FUNCTION_STUB(DWORD, NULL, BaseTexture::GetLOD)

FUNCTION_STUB(DWORD, NULL, BaseTexture::GetLevelCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, BaseTexture::SetAutoGenFilterType, D3DTEXTUREFILTERTYPE FilterType)

FUNCTION_STUB(D3DTEXTUREFILTERTYPE, static_cast<D3DTEXTUREFILTERTYPE>(NULL), BaseTexture::GetAutoGenFilterType)

FUNCTION_STUB(void, , BaseTexture::GenerateMipSubLevels)

FUNCTION_STUB(void, , BaseTexture::FUN_48)

FUNCTION_STUB(HRESULT, E_NOTIMPL, BaseTexture::FUN_4C, void* A1)