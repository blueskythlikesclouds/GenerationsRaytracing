#pragma once

#include "Texture.h"
#include "Window.h"

class Device;
struct MsgCreateSwapChain;

class SwapChain
{
protected:
    ComPtr<IDXGIFactory4> m_factory;
    Window m_window;
    ComPtr<IDXGISwapChain3> m_swapChain;
    uint8_t m_syncInterval = 0;
    std::vector<Texture> m_textures;
    std::vector<ComPtr<ID3D12Resource>> m_resources;

public:
    SwapChain();

    IDXGIFactory4* getUnderlyingFactory() const;
    Window& getWindow();
    const Texture& getTexture() const;
    ID3D12Resource* getResource() const;

    void present() const;

    void procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message);
};
