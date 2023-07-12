#include "Texture.h"

#include "Message.h"
#include "MessageSender.h"
#include "Surface.h"

Texture::Texture(uint32_t width, uint32_t height, uint32_t levelCount)
    : BaseTexture(levelCount), m_width(width), m_height(height)
{
}

Texture::~Texture() = default;

uint32_t Texture::getWidth() const
{
    return m_width;
}

uint32_t Texture::getHeight() const
{
    return m_height;
}

HRESULT Texture::GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc)
{
    pDesc->Format = D3DFMT_UNKNOWN;
    pDesc->Type = D3DRTYPE_SURFACE;
    pDesc->Usage = NULL;
    pDesc->Pool = D3DPOOL_DEFAULT;
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    pDesc->MultiSampleQuality = 0;
    pDesc->Width = m_width >> Level;
    pDesc->Height = m_height >> Level;

    return S_OK;
}

HRESULT Texture::GetSurfaceLevel(UINT Level, Surface** ppSurfaceLevel)
{
    if (!m_surfaces[Level])
        m_surfaces[Level].Attach(new Surface(this, Level));

    m_surfaces[Level].CopyTo(ppSurfaceLevel);
    return S_OK;
}

HRESULT Texture::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags)
{
    // TODO: is it safe to assume RGBA8 here??
    auto& message = s_messageSender.makeParallelMessage<MsgWriteTexture>((m_width >> Level) * (m_height >> Level) * 4);

    message.textureId = m_id;
    message.level = Level;

    pLockedRect->pBits = message.data;
    pLockedRect->Pitch = static_cast<INT>((m_width >> Level) * 4);

    return S_OK;
}

HRESULT Texture::UnlockRect(UINT Level)
{
    s_messageSender.endParallelMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Texture::AddDirtyRect, const RECT* pDirtyRect)