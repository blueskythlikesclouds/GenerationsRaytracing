#include "Texture.h"
#include "Surface.h"

HRESULT Texture::GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc)
{
    return S_OK;
}

HRESULT Texture::GetSurfaceLevel(UINT Level, Surface** ppSurfaceLevel)
{
    *ppSurfaceLevel = reinterpret_cast<Surface*>(this);
    (*ppSurfaceLevel)->AddRef();
    return S_OK;
}

HRESULT Texture::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
    return S_OK;
}

HRESULT Texture::UnlockRect(UINT Level)
{
    return S_OK;
}

FUNCTION_STUB(HRESULT, Texture::AddDirtyRect, CONST RECT* pDirtyRect)

HRESULT Texture::BeginSfdDecodeCallback(uintptr_t, Texture*& texture, uintptr_t, uintptr_t)
{
    texture = this;
    return S_OK;
}

HRESULT Texture::EndSfdDecodeCallback(uintptr_t)
{
    return S_OK;
}