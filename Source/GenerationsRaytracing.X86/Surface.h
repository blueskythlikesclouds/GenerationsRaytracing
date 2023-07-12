#pragma once

#include "Resource.h"

class Texture;

class Surface : public Resource
{
protected:
    ComPtr<Texture> m_texture;
    uint32_t m_level;

public:
    explicit Surface(Texture* texture, size_t level);
    ~Surface() override;

    Texture* getTexture() const;
    uint32_t getLevel() const;

    virtual HRESULT GetContainer(const IID& riid, void** ppContainer) final;
    virtual HRESULT GetDesc(D3DSURFACE_DESC* pDesc) final;
    virtual HRESULT LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) final;
    virtual HRESULT UnlockRect() final;
    virtual HRESULT GetDC(HDC* phdc) final;
    virtual HRESULT ReleaseDC(HDC hdc) final;
};