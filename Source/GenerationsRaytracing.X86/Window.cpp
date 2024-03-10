#include "Window.h"

#include "Configuration.h"
#include "RaytracingParams.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

    switch (Msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_F1: 
            Configuration::s_enableImgui ^= true;
            break;

        case VK_F2:
            RaytracingParams::s_enable ^= true;
            break;
        }
        break;
    }

    if (Configuration::s_enableImgui)
        ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

    return reinterpret_cast<WNDPROC>(0xE7B6C0)(hWnd, Msg, wParam, lParam);
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
        HWND hWnd = CreateWindowEx(
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

        if (hWnd)
        {
            SetWindowLong(hWnd, GWL_USERDATA, TRUE);

            *reinterpret_cast<HWND*>(param + 72) = hWnd;
            *reinterpret_cast<bool*>(param + 144) = true;

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