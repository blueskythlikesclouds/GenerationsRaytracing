#include "Buffer.h"

#include "Message.h"
#include "MessageSender.h"

Buffer::Buffer(size_t length) : length(length)
{
}

HRESULT Buffer::Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags)
{
    const size_t size = SizeToLock == 0 ? length : SizeToLock;

    const auto msg = msgSender.start<MsgWriteBuffer>(size);

    msg->buffer = (unsigned int)this;
    msg->size = size;

    *ppbData = MSG_DATA_PTR(msg);

    return S_OK;
}

HRESULT Buffer::Unlock()
{
    msgSender.finish();
    return S_OK;
}

FUNCTION_STUB(HRESULT, Buffer::GetDesc, D3DINDEXBUFFER_DESC *pDesc)