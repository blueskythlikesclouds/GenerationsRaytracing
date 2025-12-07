#pragma once

#include "Event.h"
#include "Texture.h"
#include "Window.h"

class Device;
struct MsgCreateSwapChain;

class SwapChain
{
protected:
    ComPtr<IDXGIFactory6> m_factory;
    Window m_window;
    ComPtr<IDXGISwapChain4> m_swapChain;
    HANDLE m_waitableObject{};
    bool m_pendingWait = true;
    uint8_t m_syncInterval = 0;
    std::vector<Texture> m_textures;
    std::vector<ComPtr<ID3D12Resource>> m_resources;
    ffx::Context m_context{};

public:
    SwapChain();

    IDXGIFactory6* getUnderlyingFactory() const;
    Window& getWindow();
    IDXGISwapChain4* getUnderlyingSwapChain() const;
    const Texture& getTexture() const;
    ID3D12Resource* getResource() const;

    void wait();
    void present();

    void procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message);

    void replaceSwapChainForFrameInterpolation(const Device& device);
};
