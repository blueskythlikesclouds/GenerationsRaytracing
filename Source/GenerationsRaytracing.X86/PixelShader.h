#pragma once

#include "Unknown.h"

class Device;

class PixelShader : public Unknown
{
protected:
    uint32_t m_id;
public:
    PixelShader();

    uint32_t getId() const;

    virtual HRESULT GetDevice(Device** ppDevice) final;
    virtual HRESULT GetFunction(void*, UINT* pSizeOfData) final;
};
