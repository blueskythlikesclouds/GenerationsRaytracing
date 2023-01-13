#pragma once

struct Bridge;
struct MsgInitSwapChain;

struct Window
{
    HWND handle{};
    HWND gensHandle{};
    bool mouseTracked = false;

    ~Window();

    void procMsgInitSwapChain(Bridge& bridge, const MsgInitSwapChain& msg);
    void processMessages() const;
};
