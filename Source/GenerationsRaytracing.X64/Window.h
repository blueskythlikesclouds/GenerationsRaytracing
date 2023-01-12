#pragma once

struct MsgInitSwapChain;
struct Bridge;

struct Window
{
    HWND handle{};
    HWND gensHandle{};

    ~Window();

    void init(Bridge* bridge, const MsgInitSwapChain& msg);
    void processMessages() const;
};
