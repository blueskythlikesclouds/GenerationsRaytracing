#include "Surface.h"
#include "Texture.h"

Surface::Surface(Texture* texture, size_t level)
    : m_texture(texture), m_level(level)
{
}

Surface::~Surface() = default;

Texture* Surface::getTexture() const
{
    return m_texture.Get();
}

uint32_t Surface::getLevel() const
{
    return m_level;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Surface::GetContainer, const IID& riid, void** ppContainer)

HRESULT Surface::GetDesc(D3DSURFACE_DESC* pDesc)
{
    pDesc->Format = D3DFMT_UNKNOWN;
    pDesc->Type = D3DRTYPE_SURFACE;
    pDesc->Usage = NULL;
    pDesc->Pool = D3DPOOL_DEFAULT;
    pDesc->MultiSampleType = D3DMULTISAMPLE_NONE;
    pDesc->MultiSampleQuality = 0;
    pDesc->Width = m_texture->getWidth() >> m_level;
    pDesc->Height = m_texture->getHeight() >> m_level;

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Surface::LockRect, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Surface::UnlockRect)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Surface::GetDC, HDC* phdc)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Surface::ReleaseDC, HDC hdc)