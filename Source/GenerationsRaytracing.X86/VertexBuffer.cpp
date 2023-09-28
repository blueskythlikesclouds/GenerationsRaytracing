#include "VertexBuffer.h"

#include "Message.h"
#include "MessageSender.h"
#include "FreeListAllocator.h"

static FreeListAllocator s_freeListAllocator;

VertexBuffer::VertexBuffer(uint32_t byteSize)
{
    m_id = s_freeListAllocator.allocate();
    m_byteSize = byteSize;
}

VertexBuffer::~VertexBuffer()
{
    s_freeListAllocator.free(m_id);
}

uint32_t VertexBuffer::getId() const
{
    return m_id;
}

HRESULT VertexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (SizeToLock == 0)
        SizeToLock = m_byteSize - OffsetToLock;

    auto& message = s_messageSender.makeParallelMessage<MsgWriteVertexBuffer>(SizeToLock);

    message.vertexBufferId = m_id;
    message.offset = OffsetToLock;
    *ppbData = message.data;

    return S_OK;
}

HRESULT VertexBuffer::Unlock()
{
    s_messageSender.endParallelMessage();

    return S_OK;
}

HRESULT VertexBuffer::GetDesc(D3DVERTEXBUFFER_DESC* pDesc)
{
    pDesc->Format = D3DFMT_UNKNOWN;
    pDesc->Type = D3DRTYPE_VERTEXBUFFER;
    pDesc->Usage = NULL;
    pDesc->Pool = D3DPOOL_DEFAULT;
    pDesc->Size = m_byteSize;
    pDesc->FVF = NULL;

    return S_OK;
}