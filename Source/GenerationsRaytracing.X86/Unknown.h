#pragma once

class Unknown
{
protected:
    std::atomic<ULONG> refCount = 1;

public:
    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj) final;

    virtual ULONG AddRef() final;

    virtual ULONG Release() final;

    virtual ~Unknown();
};