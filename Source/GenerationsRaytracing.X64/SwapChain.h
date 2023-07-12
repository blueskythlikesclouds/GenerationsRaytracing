#pragma once

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

public:
    SwapChain();

    IDXGIAdapter* getAdapter() const;

    void procMsgCreateSwapChain(const Device& device, const MsgCreateSwapChain& message);
};
