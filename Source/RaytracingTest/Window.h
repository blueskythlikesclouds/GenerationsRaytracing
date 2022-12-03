#pragma once

#include "Input.h"

struct Device;

struct Window
{
    int width = 0;
    int height = 0;
    HWND handle = nullptr;
    Input input;

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

    Window(const Device& device, int width, int height);

    void processMessages();
    void present() const;

    nvrhi::ITexture* getCurrentSwapChainTexture() const;
    nvrhi::IFramebuffer* getCurrentSwapChainFramebuffer() const;
};

