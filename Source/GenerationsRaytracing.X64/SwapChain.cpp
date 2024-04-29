#include "SwapChain.h"

#include "Device.h"
#include "DxgiConverter.h"
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

    if (FAILED(hr))
        MessageBox(nullptr, TEXT("Failed to create DXGI factory."), TEXT("Generations Raytracing"), MB_ICONERROR);
}

IDXGIFactory4* SwapChain::getUnderlyingFactory() const
{
    return m_factory.Get();
}

Window& SwapChain::getWindow()
{
    return m_window;
}

IDXGISwapChain4* SwapChain::getUnderlyingSwapChain() const
{
    return m_swapChain.Get();
}

const Texture& SwapChain::getTexture() const
{
    return m_textures[m_swapChain->GetCurrentBackBufferIndex()];
}

ID3D12Resource* SwapChain::getResource() const
{
    return m_resources[m_swapChain->GetCurrentBackBufferIndex()].Get();
}

void SwapChain::present() const
{
    const HRESULT hr = m_swapChain->Present(m_syncInterval, 0);
    assert(SUCCEEDED(hr));
}

void SwapChain::procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message)
{
    m_window.procMsgCreateSwapChain(message);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = message.renderWidth;
    desc.Height = message.renderHeight;
    desc.Format = DxgiConverter::convert(static_cast<D3DFORMAT>(message.format));
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = message.bufferCount;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

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

    m_syncInterval = message.syncInterval;

    for (uint32_t i = 0; i < message.bufferCount; i++)
    {
        auto& texture = m_textures.emplace_back();
        auto& resource = m_resources.emplace_back();

        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(resource.GetAddressOf()));

        assert(SUCCEEDED(hr) && resource != nullptr);

#ifdef _DEBUG
        wchar_t name[0x100];
        swprintf_s(name, L"Swap Chain %d", i);
        resource->SetName(name);
#endif

        texture.format = resource->GetDesc().Format;
        texture.rtvIndex = device.getRtvDescriptorHeap().allocate();

        device.getUnderlyingDevice()->CreateRenderTargetView(
            resource.Get(), nullptr, device.getRtvDescriptorHeap().getCpuHandle(texture.rtvIndex));
    }
}

void SwapChain::replaceSwapChainForFrameInterpolation(const Device& device)
{
    if (!m_frameInterpolation)
    {
        for (auto& resource : m_resources)
            resource = nullptr;

        FfxCommandQueue commandQueue = ffxGetCommandQueueDX12(device.getGraphicsQueue().getUnderlyingQueue());
        FfxSwapchain swapChain = ffxGetSwapchainDX12(m_swapChain.Detach());
        ffxReplaceSwapchainForFrameinterpolationDX12(commandQueue, swapChain);
        m_swapChain.Attach(ffxGetDX12SwapchainPtr(swapChain));

        for (uint32_t i = 0; i < m_textures.size(); i++)
        {
            auto& resource = m_resources[i];
            HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(resource.GetAddressOf()));
            assert(SUCCEEDED(hr) && resource != nullptr);

            device.getUnderlyingDevice()->CreateRenderTargetView(
                resource.Get(), nullptr, device.getRtvDescriptorHeap().getCpuHandle(m_textures[i].rtvIndex));
        }

        m_frameInterpolation = true;
    }
}
