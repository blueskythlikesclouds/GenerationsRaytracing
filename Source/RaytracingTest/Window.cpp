#include "Window.h"

#include "Device.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    Window* window = (Window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (window->input.wndProc(hWnd, Msg, wParam, lParam))
        return 0;

    switch (Msg)
    {
    case WM_CLOSE:
        window->shouldClose = true;
        return 0;
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

Window::Window(const Device& device, int width, int height)
    : width(width), height(height)
{
    WNDCLASSEX wndClassEx {};

    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClassEx.lpszClassName = TEXT("RaytracingTest");

    RegisterClassEx(&wndClassEx);

    handle = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        wndClassEx.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        width,
        height,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    assert(handle);

    SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)this);

    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();

    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(handle);
    ShowWindow(handle, SW_SHOW);
    AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = (UINT)width;
    swapChainDesc.Height = (UINT)height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain;
    device.dxgiFactory->CreateSwapChainForHwnd(device.d3d12.graphicsCommandQueue.Get(), handle, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
    assert(swapChain);

    swapChain.As(&dxgi.swapChain);
    assert(dxgi.swapChain);

    for (unsigned int i = 0; i < swapChainDesc.BufferCount; i++)
    {
        ComPtr<ID3D12Resource> backBuffer;
        dxgi.swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        assert(backBuffer);

        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setWidth(swapChainDesc.Width)
            .setHeight(swapChainDesc.Height)
            .setIsRenderTarget(true)
            .setInitialState(nvrhi::ResourceStates::Present)
            .setKeepInitialState(true);

        nvrhi.swapChainTextures.push_back(device.nvrhi->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, backBuffer.Get(), textureDesc));
        assert(nvrhi.swapChainTextures.back());

        const auto framebufferDesc = nvrhi::FramebufferDesc()
            .addColorAttachment(nvrhi.swapChainTextures.back());

        nvrhi.swapChainFramebuffers.push_back(device.nvrhi->createFramebuffer(framebufferDesc));
        assert(nvrhi.swapChainFramebuffers.back());
    }
}

void Window::processMessages()
{
    input.update();

    MSG msg;
    while (PeekMessage(&msg, handle, 0, 0, TRUE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Window::present() const
{
    dxgi.swapChain->Present(1, 0);
}

nvrhi::ITexture* Window::getCurrentSwapChainTexture() const
{
    return nvrhi.swapChainTextures[dxgi.swapChain->GetCurrentBackBufferIndex()];
}

nvrhi::IFramebuffer* Window::getCurrentSwapChainFramebuffer() const
{
    return nvrhi.swapChainFramebuffers[dxgi.swapChain->GetCurrentBackBufferIndex()];
}
