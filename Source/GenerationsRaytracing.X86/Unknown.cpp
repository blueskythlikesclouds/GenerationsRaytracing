#include "Unknown.h"

#include "Message.h"
#include "MessageSender.h"

HRESULT Unknown::QueryInterface(const IID& riid, void** ppvObj)
{
    // Not used by game.
    return S_OK;
}

ULONG Unknown::AddRef()
{
    return ++refCount;
}

ULONG Unknown::Release()
{
    const ULONG result = --refCount;
    if (result == 0)
        delete this;

    return result;
}

Unknown::~Unknown()
{
    const auto msg = msgSender.start<MsgReleaseResource>();
    msg->resource = (unsigned int)this;
    msgSender.finish();
}
