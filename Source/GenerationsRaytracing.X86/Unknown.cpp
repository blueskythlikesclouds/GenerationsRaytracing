#include "Unknown.h"

Unknown::Unknown() : refCount(1)
{
}

HRESULT Unknown::QueryInterface(const IID& riid, void** ppvObj)
{
    // Not used by game.
    return S_OK;
}

ULONG Unknown::AddRef()
{
    return InterlockedIncrement(&refCount);
}

ULONG Unknown::Release()
{
    const ULONG result = InterlockedDecrement(&refCount);
    if (result == 0)
        delete this;

    return result;
}

Unknown::~Unknown() = default;
