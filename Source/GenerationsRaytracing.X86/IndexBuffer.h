#pragma once

#include "Resource.h"

class IndexBuffer : public Resource
{
public:
protected:
    uint32_t m_id;
    uint32_t m_byteSize;

public:
    explicit IndexBuffer(uint32_t byteSize);
    ~IndexBuffer() override;

    uint32_t getId() const;

    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) final;
    virtual HRESULT Unlock() final;
    virtual HRESULT GetDesc(D3DINDEXBUFFER_DESC* pDesc) final;
};
