#pragma once

#include "Resource.h"

class Buffer : public Resource
{
protected:
    size_t length;
public:
    Buffer(size_t length);

    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) final;
    virtual HRESULT Unlock() final;
    virtual HRESULT GetDesc(D3DINDEXBUFFER_DESC* pDesc) final;
};