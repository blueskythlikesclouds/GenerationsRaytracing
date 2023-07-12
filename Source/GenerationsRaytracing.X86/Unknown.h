#pragma once

class Unknown
{
protected:
    std::atomic<ULONG> m_refCount;

public:
    Unknown();

    Unknown(Unknown&&) = delete;
    Unknown(const Unknown&) = delete;

    virtual HRESULT QueryInterface(REFIID riid, void** ppvObj) final;

    virtual ULONG AddRef() final;
    virtual ULONG Release() final;

    virtual ~Unknown();
};
