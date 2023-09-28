#include "SwapChain.h"

#include "Device.h"
#include "Message.h"

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

Window& SwapChain::getWindow()
{
    return m_window;
}

const Texture& SwapChain::getTexture() const
{
    return m_textures[m_swapChain->GetCurrentBackBufferIndex()];
}

void SwapChain::present() const
{
    const HRESULT hr = m_swapChain->Present(0, 0);
    assert(SUCCEEDED(hr));
}

void SwapChain::procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message)
{
    m_window.procMsgCreateSwapChain(message);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = message.renderWidth;
    desc.Height = message.renderHeight;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = message.bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

    ComPtr<IDXGISwapChain1> swapChain;

    HRESULT hr = m_factory->CreateSwapChainForHwnd(
        device.getGraphicsQueue().getUnderlyingQueue(),
        m_window.getHandle(),
        &desc,
        nullptr,
        nullptr,
        swapChain.GetAddressOf());

    assert(SUCCEEDED(hr) && swapChain != nullptr);

    hr = swapChain.As(&m_swapChain);

    assert(SUCCEEDED(hr) && m_swapChain != nullptr);

    for (uint32_t i = 0; i < message.bufferCount; i++)
    {
        auto& texture = m_textures.emplace_back();
        ComPtr<ID3D12Resource> resource;

        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(resource.GetAddressOf()));

        assert(SUCCEEDED(hr) && resource != nullptr);

        texture.rtvIndex = device.getRtvDescriptorHeap().allocate();

        device.getUnderlyingDevice()->CreateRenderTargetView(
            resource.Get(), nullptr, device.getRtvDescriptorHeap().getCpuHandle(texture.rtvIndex));
    }
}
