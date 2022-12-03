#pragma once

struct Input
{
    bool key[256] {};
    bool leftMouse = false;
    bool rightMouse = false;
    int cursorX = 0;
    int cursorY = 0;
    int prevCursorX = 0;
    int prevCursorY = 0;

    bool wndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    void update();
};
