#pragma once

#include "BaseTexture.h"

class Surface;

class Texture : public BaseTexture
{
public:
    size_t width = 0;
    size_t height = 0;
    ComPtr<Surface> surface;

    Texture();
    Texture(size_t width, size_t height);
    ~Texture();

    virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) final;
    virtual HRESULT GetSurfaceLevel(UINT Level, Surface** ppSurfaceLevel);
    virtual HRESULT LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) final;
    virtual HRESULT UnlockRect(UINT Level) final;
    virtual HRESULT AddDirtyRect(const RECT* pDirtyRect) final;

    // The virtual function addresses are patched to point at those for SFD playback.
    virtual HRESULT BeginSfdDecodeCallback(uintptr_t, Texture*& texture, uintptr_t, uintptr_t) final;
    virtual HRESULT EndSfdDecodeCallback(uintptr_t) final;
};