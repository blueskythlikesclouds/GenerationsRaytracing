#include "Window.h"

#include "Configuration.h"
#include "Event.h"
#include "RaytracingParams.h"
#include "Message.h"
#include "MessageSender.h"

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
        case VK_F2: 
            Configuration::s_enableImgui ^= true;
            break;

        case VK_F3:
            RaytracingParams::s_enable ^= true;
            break;
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        Im3d::GetAppData().m_keyDown[Im3d::Mouse_Left] = (Msg == WM_LBUTTONDOWN);
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

HOOK(void*, __fastcall, ProcessWindowMessages, 0xE7BED0, void* This, void* _, void* a2, void* a3) 
{
    s_messageSender.oneShotMessage<MsgProcessWindowMessages>();
    s_messageSender.sync();

    return originalProcessWindowMessages(This, _, a2, a3);
}

static Event s_swapChainEvent{ Event::s_swapChainEventName, FALSE, FALSE };

HOOK(void*, __stdcall, SampleInput, 0xD683C0, float a1)
{
    if (!(*s_shouldExit))
    {
        s_messageSender.oneShotMessage<MsgWaitOnSwapChain>();
        s_swapChainEvent.wait();
    }

    return originalSampleInput(a1);
}

void Window::init()
{
    WRITE_JUMP(0xE7B810, createWindowTrampoline);

    // SetCooperativeLevel flags
    WRITE_MEMORY(0x9C922D, uint8_t, 0xC);
    WRITE_MEMORY(0x9C96DD, uint8_t, 0xC);

    INSTALL_HOOK(ProcessWindowMessages);
    INSTALL_HOOK(SampleInput);
}

void Window::notifyShouldExit()
{
    s_swapChainEvent.set();
}
