#pragma once

struct Bridge;
struct MsgInitSwapChain;

struct Window
{
    HWND handle{};
    HWND gensHandle{};
    bool mouseTracked = false;

    ~Window();

    void procMsgInitWindow(Bridge& bridge);
    void procMsgInitSwapChain(Bridge& bridge, const MsgInitSwapChain& msg);

    void processMessages() const;
};
