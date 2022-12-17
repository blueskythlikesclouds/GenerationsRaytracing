#pragma once

#include "Unknown.h"

class Device;

class Resource : public Unknown
{
public:
    virtual HRESULT GetDevice(Device** ppDevice) final;
    virtual HRESULT SetPrivateData(const GUID& refguid, const void* pData, DWORD SizeOfData, DWORD Flags) final;
    virtual HRESULT GetPrivateData(const GUID& refguid, void* pData, DWORD* pSizeOfData) final;
    virtual HRESULT FreePrivateData(const GUID& refguid) final;
    virtual DWORD SetPriority(DWORD PriorityNew) final;
    virtual DWORD GetPriority() final;
    virtual void PreLoad() final;
    virtual D3DRESOURCETYPE GetType() final;
};
