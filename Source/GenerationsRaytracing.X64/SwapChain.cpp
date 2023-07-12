#include "SwapChain.h"

SwapChain::SwapChain()
{
    HRESULT hr = CreateDXGIFactory2(
#ifdef _DEBUG
        DXGI_CREATE_FACTORY_DEBUG,
#else
        0,
#endif
        IID_PPV_ARGS(m_factory.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_factory != nullptr);

    hr = m_factory->EnumAdapters(0, m_adapter.GetAddressOf());

    assert(SUCCEEDED(hr) && m_adapter != nullptr);
}

IDXGIAdapter* SwapChain::getAdapter() const
{
    return m_adapter.Get();
}

void SwapChain::procMsgCreateSwapChain(const Device& device, const MsgCreateSwapChain& message)
{
    m_window.procMsgCreateSwapChain(message);
}
