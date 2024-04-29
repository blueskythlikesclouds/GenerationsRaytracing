#pragma once

#include "BaseTexture.h"

class Surface;

class Texture : public BaseTexture
{
protected:
    uint32_t m_width;
    uint32_t m_height;
    ComPtr<Surface> m_surfaces[15];

public:
    explicit Texture(uint32_t width, uint32_t height, uint32_t levelCount);
    ~Texture() override;

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    Surface* getSurface(size_t index);

    void setResolution(uint32_t width, uint32_t height);

    virtual HRESULT GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) final;
    virtual HRESULT GetSurfaceLevel(UINT Level, Surface** ppSurfaceLevel);
    virtual HRESULT LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) final;
    virtual HRESULT UnlockRect(UINT Level) final;
    virtual HRESULT AddDirtyRect(const RECT* pDirtyRect) final;

    // The virtual function addresses are patched to point at those for SFD playback.
    virtual HRESULT BeginSfdDecodeCallback(uintptr_t, Texture*& texture, uintptr_t, uintptr_t) final;
    virtual HRESULT EndSfdDecodeCallback(uintptr_t) final;
};
