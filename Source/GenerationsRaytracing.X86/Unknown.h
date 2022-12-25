#pragma once

class Unknown
{
public:
    std::atomic<ULONG> refCount = 1;

    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj) final;

    virtual ULONG AddRef() final;

    virtual ULONG Release() final;

    virtual ~Unknown();
};