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
            bridge->breakMessageLoop();
            goto postMessage;

        case WM_MOUSEMOVE:
            if (!bridge->window.mouseTracked)
            {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                bridge->window.mouseTracked = true;
            }
            goto postMessage;

        case WM_MOUSELEAVE:
            bridge->window.mouseTracked = false;
            goto postMessage;

        case WM_ACTIVATE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_CHAR:
        case WM_SETCURSOR:
        case WM_DEVICECHANGE:
        postMessage:
            PostMessage(bridge->window.gensHandle, WM_USER + Msg, wParam, lParam);
            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

Window::~Window()
{
    CloseHandle(handle);
}

void Window::procMsgInitSwapChain(Bridge& bridge, const MsgInitSwapChain& msg)
{
    gensHandle = (HWND)(LONG_PTR)msg.handle;

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
        TEXT("SEGA - Sonic Generations"),
        msg.style,
        msg.x,
        msg.y,
        msg.width,
        msg.height,
        nullptr,
        nullptr,
        wndClassEx.hInstance,
        nullptr);

    SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR)&bridge);

    assert(handle);

    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();

    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(handle);
    ShowWindow(handle, SW_SHOW);
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
