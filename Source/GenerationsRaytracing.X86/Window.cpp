#include "Window.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    printf("Received message %x %x %x\n", Msg, wParam, lParam);

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
    WNDCLASSEXA wndClassEx{};
    wndClassEx.cbSize = sizeof(WNDCLASSEXA);
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = hInstance;
    wndClassEx.lpszClassName = lpszClassName;

    if (RegisterClassExA(&wndClassEx))
    {
        auto& handle = *(HWND*)(param + 72);

        handle = CreateWindowExA(
            0,
            lpszClassName,
            lpWindowName,
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
}
