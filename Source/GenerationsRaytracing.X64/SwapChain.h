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
    ComPtr<IDXGISwapChain4> m_swapChain;
    uint8_t m_syncInterval = 0;
    std::vector<Texture> m_textures;
    std::vector<ComPtr<ID3D12Resource>> m_resources;
    bool m_frameInterpolation = false;

public:
    SwapChain();

    IDXGIFactory4* getUnderlyingFactory() const;
    Window& getWindow();
    IDXGISwapChain4* getSwapChain() const;
    const Texture& getTexture() const;
    ID3D12Resource* getResource() const;

    void present() const;

    void procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message);

    void replaceSwapChainForFrameInterpolation(const Device& device);
};
