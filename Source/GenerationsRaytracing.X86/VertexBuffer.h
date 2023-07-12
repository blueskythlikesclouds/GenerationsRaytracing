#pragma once

#include "Resource.h"

class VertexBuffer : public Resource
{
public:
protected:
    uint32_t m_id;
    uint32_t m_byteSize;

public:
    explicit VertexBuffer(uint32_t byteSize);
    ~VertexBuffer() override;

    uint32_t getId() const;

    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) final;
    virtual HRESULT Unlock() final;
    virtual HRESULT GetDesc(D3DVERTEXBUFFER_DESC* pDesc) final;
};
