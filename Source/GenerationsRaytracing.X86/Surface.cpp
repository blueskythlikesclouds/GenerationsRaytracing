#include "Surface.h"

#include "Texture.h"

Surface::Surface() = default;

Surface::Surface(Texture* texture) : texture(texture)
{
}

Surface::~Surface() = default;

FUNCTION_STUB(HRESULT, Surface::GetContainer, REFIID riid, void** ppContainer)

HRESULT Surface::GetDesc(D3DSURFACE_DESC *pDesc)
{
    assert(texture);

    pDesc->Width = texture->width;
    pDesc->Height = texture->height;

    return S_OK;
}

FUNCTION_STUB(HRESULT, Surface::LockRect, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)

FUNCTION_STUB(HRESULT, Surface::UnlockRect)

FUNCTION_STUB(HRESULT, Surface::GetDC, HDC *phdc)

FUNCTION_STUB(HRESULT, Surface::ReleaseDC, HDC hdc)

