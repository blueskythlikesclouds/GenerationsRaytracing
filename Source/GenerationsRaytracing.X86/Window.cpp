#include "Window.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg < WM_USER)
    {
        if (GetWindowLong(hWnd, GWL_USERDATA) != 0)
            return 0;
    }
    else
    {
        Msg -= WM_USER;
    }

    return ((WNDPROC)0xE7B6C0)(hWnd, Msg, wParam, lParam);
}

static bool __fastcall createWindowMidAsmHook(
    const char* lpszClassName,
    size_t param,
    HINSTANCE hInstance,
    const char* lpWindowName,
    int width,
    int height)
{
    WNDCLASSEX wndClassEx{};
    wndClassEx.cbSize = sizeof(WNDCLASSEXA);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = TEXT("SonicGenerations");

    if (RegisterClassEx(&wndClassEx))
    {
        HWND handle = CreateWindowEx(
            0,
            wndClassEx.lpszClassName,
            wndClassEx.lpszClassName,
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            hInstance,
            (LPVOID)param);

        if (handle)
        {
            SetWindowLong(handle, GWL_USERDATA, TRUE);

            *(HWND*)(param + 72) = handle;
            *(bool*)(param + 144) = true;

            return true;
        }
    }

    return false;
}

static void __declspec(naked) createWindowTrampoline()
{
    __asm
    {
        mov ecx, edi
        mov edx, esi
        jmp createWindowMidAsmHook
    }
}

void Window::init()
{
    WRITE_JUMP(0xE7B810, createWindowTrampoline);

    // SetCooperativeLevel flags
    WRITE_MEMORY(0x9C922D, uint8_t, 0xC);
    WRITE_MEMORY(0x9C96DD, uint8_t, 0xC);
}