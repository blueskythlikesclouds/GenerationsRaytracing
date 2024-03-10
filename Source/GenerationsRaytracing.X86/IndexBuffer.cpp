#include "IndexBuffer.h"

#include "Message.h"
#include "MessageSender.h"
#include "FreeListAllocator.h"

static FreeListAllocator s_idAllocator;

IndexBuffer::IndexBuffer(uint32_t byteSize)
{
    m_id = s_idAllocator.allocate();
    m_byteSize = byteSize;
}

IndexBuffer::~IndexBuffer()
{
    auto& message = s_messageSender.makeMessage<MsgReleaseResource>();

    message.resourceType = MsgReleaseResource::ResourceType::IndexBuffer;
    message.resourceId = m_id;

    s_messageSender.endMessage();

    s_idAllocator.free(m_id);
}

uint32_t IndexBuffer::getId() const
{
    return m_id;
}

uint32_t IndexBuffer::getByteSize() const
{
    return m_byteSize;
}

HRESULT IndexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (SizeToLock == 0)
        SizeToLock = m_byteSize - OffsetToLock;

    auto& message = s_messageSender.makeMessage<MsgWriteIndexBuffer>(SizeToLock);

    message.indexBufferId = m_id;
    message.offset = OffsetToLock;
    message.initialWrite = m_pendingWrite;
    *ppbData = message.data;

    m_pendingWrite = false;

    return S_OK;
}

HRESULT IndexBuffer::Unlock()
{
    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, IndexBuffer::GetDesc, D3DINDEXBUFFER_DESC* pDesc)