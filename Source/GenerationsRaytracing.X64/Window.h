#pragma once

struct MsgCreateSwapChain;
struct MsgShowCursor;

class Window
{
protected:
    HWND m_handle{};
    HWND m_postHandle{};
    bool m_mouseTracked = false;
    bool m_showCursor{};

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

public:
    bool m_shouldExit = false;

    void procMsgCreateSwapChain(const MsgCreateSwapChain& message);
    void procMsgShowCursor(const MsgShowCursor& message);

    HWND getHandle() const;

    void processMessages() const;
};