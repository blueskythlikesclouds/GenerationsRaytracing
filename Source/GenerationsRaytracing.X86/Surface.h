#pragma once

#include "Resource.h"

class Texture;

class Surface : public Resource
{
public:
    ComPtr<Texture> texture;

    Surface();
    Surface(Texture* texture);
    ~Surface();

    virtual HRESULT GetContainer(const IID& riid, void** ppContainer) final;
    virtual HRESULT GetDesc(D3DSURFACE_DESC* pDesc) final;
    virtual HRESULT LockRect(D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD Flags) final;
    virtual HRESULT UnlockRect() final;
    virtual HRESULT GetDC(HDC* phdc) final;
    virtual HRESULT ReleaseDC(HDC hdc) final;
};
