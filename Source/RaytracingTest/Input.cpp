#include "Input.h"

bool Input::wndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_KEYDOWN:
        if (wParam < std::size(key))
            key[wParam] = true;

        break;

    case WM_KEYUP:
        if (wParam < std::size(key))
            key[wParam] = false;

        break;

    case WM_MOUSEMOVE:
        cursorX = GET_X_LPARAM(lParam);
        cursorY = GET_Y_LPARAM(lParam);
        break;

    case WM_LBUTTONDOWN:
        leftMouse = true;
        break;

    case WM_LBUTTONUP:
        leftMouse = false;
        break;

    case WM_RBUTTONDOWN:
        rightMouse = true;
        break;

    case WM_RBUTTONUP:
        rightMouse = false;
        break;

    default:
        return false;
    }

    return true;
}

void Input::recordHistory()
{
    prevCursorX = cursorX;
    prevCursorY = cursorY;
}
