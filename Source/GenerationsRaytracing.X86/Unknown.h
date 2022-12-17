#pragma once

class Unknown
{
protected:
    ULONG refCount;

public:
    Unknown();

    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj) final;

    virtual ULONG AddRef() final;

    virtual ULONG Release() final;

    virtual ~Unknown();
};