#pragma once

struct MsgCreateSwapChain;

class Window
{
protected:
    HWND m_handle{};
    HWND m_postHandle{};
    bool m_mouseTracked = false;

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

public:
    bool m_shouldExit = false;

    void procMsgCreateSwapChain(const MsgCreateSwapChain& message);

    HWND getHandle() const;

    void processMessages() const;
};