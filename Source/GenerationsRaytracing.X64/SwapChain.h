#pragma once

#include "Texture.h"
#include "Window.h"

class Device;
struct MsgCreateSwapChain;

class SwapChain
{
protected:
    ComPtr<IDXGIFactory4> m_factory;
    ComPtr<IDXGIAdapter> m_adapter;
    Window m_window;
    ComPtr<IDXGISwapChain3> m_swapChain;
    std::vector<Texture> m_textures;

public:
    SwapChain();

    IDXGIAdapter* getAdapter() const;
    Window& getWindow();
    const Texture& getTexture() const;

    void present() const;

    void procMsgCreateSwapChain(Device& device, const MsgCreateSwapChain& message);
};
