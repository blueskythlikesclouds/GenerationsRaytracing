#pragma once

class Unknown
{
public:
    const size_t id;
    std::atomic<ULONG> refCount;

    Unknown();

    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj) final;

    virtual ULONG AddRef() final;

    virtual ULONG Release() final;

    virtual ~Unknown();
};