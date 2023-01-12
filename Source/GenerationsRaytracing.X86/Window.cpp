#include "Window.h"

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
    wndClassEx.lpfnWndProc = (WNDPROC)0xE7B6C0;
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

    // SetCooperativeLevel flags
    WRITE_MEMORY(0x9C922D, uint8_t, 0xC);
    WRITE_MEMORY(0x9C96DD, uint8_t, 0xC);
}
