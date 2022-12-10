#pragma once

struct App;

struct Window
{
    HWND handle = nullptr;

    struct
    {
        ComPtr<IDXGISwapChain3> swapChain;
    } dxgi;

    struct
    {
        std::vector<nvrhi::TextureHandle> swapChainTextures;
        std::vector<nvrhi::FramebufferHandle> swapChainFramebuffers;
    } nvrhi;

    bool shouldClose = false;

    Window(App& app);

    void processMessages() const;
    void present() const;

    int getBackBufferIndex() const
    {
        return dxgi.swapChain->GetCurrentBackBufferIndex();
    }
};

