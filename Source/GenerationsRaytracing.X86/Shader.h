#pragma once

#include "Unknown.h"

class Device;

class Shader : public Unknown
{
public:
    virtual HRESULT GetDevice(Device** ppDevice) final;
    virtual HRESULT GetFunction(void*, UINT* pSizeOfData) final;
};
