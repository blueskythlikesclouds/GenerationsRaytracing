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

void Texture::setResolution(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
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

    const uint32_t width = m_width >> Level;
    const uint32_t height = m_height >> Level;
    const uint32_t pitch = std::max(width * 4u, 256u);

    auto& message = s_messageSender.makeMessage<MsgWriteTexture>(pitch * height);

    message.textureId = m_id;
    message.width = width;
    message.height = height;
    message.level = Level;
    message.pitch = pitch;

    pLockedRect->pBits = message.data;
    pLockedRect->Pitch = pitch;

    return S_OK;
}

HRESULT Texture::UnlockRect(UINT Level)
{
    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Texture::AddDirtyRect, const RECT* pDirtyRect)