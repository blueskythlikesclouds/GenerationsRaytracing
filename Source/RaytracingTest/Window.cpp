#include "Window.h"
#include "App.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    App* app = (App*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (app->input.wndProc(hWnd, Msg, wParam, lParam))
        return 0;

    switch (Msg)
    {
    case WM_CLOSE:
        app->window.shouldClose = true;
        return 0;
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

Window::Window(const App& app)
{
    WNDCLASSEX wndClassEx {};

    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW;
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClassEx.lpszClassName = TEXT("RaytracingTest");

    RegisterClassEx(&wndClassEx);

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);

    GetMonitorInfo(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);

    int workWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
    int workHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

    int x = monitorInfo.rcWork.left;
    int y = monitorInfo.rcWork.top;

    if (app.width > workWidth)
    {
        workWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        x = monitorInfo.rcMonitor.left;
    }

    if (app.height > workHeight)
    {
        workHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
        y = monitorInfo.rcMonitor.top;
    }

    x += (workWidth - app.width) / 2;
    y += (workHeight - app.height) / 2;

    handle = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        wndClassEx.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        x,
        y,
        app.width,
        app.height,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    assert(handle);

    RECT windowRect, clientRect;
    GetWindowRect(handle, &windowRect);
    GetClientRect(handle, &clientRect);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    int deltaX = windowWidth - clientWidth;
    int deltaY = windowHeight - clientHeight;

    SetWindowPos(
        handle,
        HWND_TOP,
        windowRect.left - deltaX / 2,
        windowRect.top - deltaY / 2, 
        app.width + deltaX, 
        app.height + deltaY,
        SWP_FRAMECHANGED);

    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();

    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(handle);
    ShowWindow(handle, SW_SHOW);
    AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);

    SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)&app);

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = (UINT)app.width;
    swapChainDesc.Height = (UINT)app.height;
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
    app.device.dxgiFactory->CreateSwapChainForHwnd(app.device.d3d12.graphicsCommandQueue.Get(), handle, &swapChainDesc, nullptr, nullptr, swapChain.GetAddressOf());
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

        nvrhi.swapChainTextures.push_back(app.device.nvrhi->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, backBuffer.Get(), textureDesc));
        assert(nvrhi.swapChainTextures.back());

        const auto framebufferDesc = nvrhi::FramebufferDesc()
            .addColorAttachment(nvrhi.swapChainTextures.back());

        nvrhi.swapChainFramebuffers.push_back(app.device.nvrhi->createFramebuffer(framebufferDesc));
        assert(nvrhi.swapChainFramebuffers.back());
    }
}

void Window::processMessages() const
{
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