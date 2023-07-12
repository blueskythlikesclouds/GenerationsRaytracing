#include "IndexBuffer.h"

#include "Message.h"
#include "MessageSender.h"
#include "SubAllocator.h"

static SubAllocator s_subAllocator;

IndexBuffer::IndexBuffer(uint32_t byteSize)
{
    m_id = s_subAllocator.allocate();
    m_byteSize = byteSize;
}

IndexBuffer::~IndexBuffer()
{
    s_subAllocator.free(m_id);
}

uint32_t IndexBuffer::getId() const
{
    return m_id;
}

HRESULT IndexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (SizeToLock == 0)
        SizeToLock = m_byteSize - OffsetToLock;

    auto& message = s_messageSender.makeParallelMessage<MsgWriteIndexBuffer>(SizeToLock);

    message.indexBufferId = m_id;
    message.offset = OffsetToLock;
    *ppbData = message.data;

    return S_OK;
}

HRESULT IndexBuffer::Unlock()
{
    s_messageSender.endParallelMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, IndexBuffer::GetDesc, D3DINDEXBUFFER_DESC* pDesc)