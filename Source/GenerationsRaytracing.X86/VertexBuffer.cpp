#include "VertexBuffer.h"

#include "Message.h"
#include "MessageSender.h"
#include "FreeListAllocator.h"

static FreeListAllocator s_idAllocator;

VertexBuffer::VertexBuffer(uint32_t byteSize)
{
    m_id = s_idAllocator.allocate();
    m_byteSize = byteSize;
}

VertexBuffer::~VertexBuffer()
{
    auto& message = s_messageSender.makeMessage<MsgReleaseResource>();

    message.resourceType = MsgReleaseResource::ResourceType::VertexBuffer;
    message.resourceId = m_id;

    s_messageSender.endMessage();

    s_idAllocator.free(m_id);
}

uint32_t VertexBuffer::getId() const
{
    return m_id;
}

HRESULT VertexBuffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    if (SizeToLock == 0)
        SizeToLock = m_byteSize - OffsetToLock;

    auto& message = s_messageSender.makeMessage<MsgWriteVertexBuffer>(SizeToLock);

    message.vertexBufferId = m_id;
    message.offset = OffsetToLock;
    message.initialWrite = m_pendingWrite;
    *ppbData = message.data;

    m_pendingWrite = false;

    return S_OK;
}

HRESULT VertexBuffer::Unlock()
{
    s_messageSender.endMessage();

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