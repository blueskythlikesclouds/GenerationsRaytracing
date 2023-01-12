#include "Window.h"

#include "Bridge.h"
#include "Message.h"
#include "Resource.h"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    Bridge* bridge = (Bridge*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (bridge != nullptr)
    {
        switch (Msg)
        {
        case WM_DESTROY:
        case WM_CLOSE:
            bridge->shouldExit = true;
            break;

        default:
            PostMessage(bridge->window.gensHandle, Msg, wParam, lParam);
            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

Window::~Window()
{
    CloseHandle(handle);
}

void Window::init(Bridge* bridge, const MsgInitSwapChain& msg)
{
    WNDCLASSEX wndClassEx{};

    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.style = CS_DBLCLKS;
    wndClassEx.lpfnWndProc = WndProc;
    wndClassEx.hInstance = GetModuleHandle(nullptr);
    wndClassEx.hIcon = LoadIcon(wndClassEx.hInstance, MAKEINTRESOURCE(IDI_ICON));
    wndClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClassEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClassEx.lpszClassName = TEXT("GenerationsRaytracing");
    wndClassEx.hIconSm = LoadIcon(wndClassEx.hInstance, MAKEINTRESOURCE(IDI_SMALLICON));

    RegisterClassEx(&wndClassEx);

    handle = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
#ifdef _DEBUG 
        TEXT("Generations Raytracing (Debug)"),
#else
        TEXT("Generations Raytracing"),
#endif
        msg.style,
        msg.x,
        msg.y,
        msg.width,
        msg.height,
        nullptr,
        nullptr,
        wndClassEx.hInstance,
        this);

    assert(handle);

    SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)bridge);

    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();

    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(handle);
    ShowWindow(handle, SW_NORMAL);
    AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);

    if (msg.style & WS_CAPTION)
    {
        RECT windowRect, clientRect;
        GetWindowRect(handle, &windowRect);
        GetClientRect(handle, &clientRect);

        uint32_t windowWidth = windowRect.right - windowRect.left;
        uint32_t windowHeight = windowRect.bottom - windowRect.top;

        uint32_t clientWidth = clientRect.right - clientRect.left;
        uint32_t clientHeight = clientRect.bottom - clientRect.top;

        uint32_t deltaX = windowWidth - clientWidth;
        uint32_t deltaY = windowHeight - clientHeight;

        SetWindowPos(
            handle, 
            HWND_TOP, 
            msg.x - deltaX / 2,
            msg.y - deltaY / 2,
            msg.width + deltaX,
            msg.height + deltaY, 
            SWP_FRAMECHANGED);
    }
}

void Window::processMessages() const
{
    if (handle == nullptr)
        return;

    MSG msg;
    while (PeekMessage(&msg, handle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
