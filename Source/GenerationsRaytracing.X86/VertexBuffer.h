#pragma once

#include "Resource.h"

class VertexBuffer : public Resource
{
protected:
    uint32_t m_id;
    uint32_t m_byteSize;
    bool m_pendingWrite = true;

public:
    static inline alignas(0x4) std::atomic<uint32_t> s_wastedMemory;

    explicit VertexBuffer(uint32_t byteSize);
    ~VertexBuffer() override;

    uint32_t getId() const;
    uint32_t getByteSize() const;

    virtual HRESULT Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) final;
    virtual HRESULT Unlock() final;
    virtual HRESULT GetDesc(D3DVERTEXBUFFER_DESC* pDesc) final;
};
