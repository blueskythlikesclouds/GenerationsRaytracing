#include "Unknown.h"

Unknown::Unknown() : m_refCount(1)
{
}

HRESULT Unknown::QueryInterface(const IID& riid, void** ppvObj)
{
    // Shall never be called.
    assert(false);
    return E_NOTIMPL;
}

ULONG Unknown::AddRef()
{
    return ++m_refCount;
}

ULONG Unknown::Release()
{
    const ULONG current = --m_refCount;

    if (current == 0)
        delete this;

    return current;
}

Unknown::~Unknown() = default;
