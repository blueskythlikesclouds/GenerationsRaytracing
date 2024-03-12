#include "Window.h"

#include "Message.h"
#include "Resource.h"

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (window != nullptr)
    {
        switch (Msg)
        {
        case WM_DESTROY:
        case WM_CLOSE:
            window->m_shouldExit = true;
            goto postMessage;

        case WM_MOUSEMOVE:
            if (!window->m_mouseTracked)
            {
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                window->m_mouseTracked = true;
            }
            goto postMessage;

        case WM_MOUSELEAVE:
            window->m_mouseTracked = false;
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
            PostMessage(window->m_postHandle, WM_USER + Msg, wParam, lParam);
            break;
        }
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

void Window::procMsgCreateSwapChain(const MsgCreateSwapChain& message)
{
    m_postHandle = reinterpret_cast<HWND>(static_cast<LONG_PTR>(message.postHandle));

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

    SetProcessDPIAware();

    m_handle = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        TEXT("SEGA - Sonic Generations"),
        WS_VISIBLE | message.style,
        message.x,
        message.y,
        message.width,
        message.height,
        nullptr,
        nullptr,
        wndClassEx.hInstance,
        nullptr);

    assert(m_handle != nullptr);

    ShowCursor(FALSE);
    SetWindowLongPtr(m_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    if (message.style & WS_CAPTION)
    {
        RECT windowRect, clientRect;
        GetWindowRect(m_handle, &windowRect);
        GetClientRect(m_handle, &clientRect);

        const LONG windowWidth = windowRect.right - windowRect.left;
        const LONG windowHeight = windowRect.bottom - windowRect.top;

        const LONG clientWidth = clientRect.right - clientRect.left;
        const LONG clientHeight = clientRect.bottom - clientRect.top;

        const LONG deltaX = windowWidth - clientWidth;
        const LONG deltaY = windowHeight - clientHeight;

        SetWindowPos(
            m_handle,
            HWND_TOP,
            message.x - deltaX / 2,
            message.y - deltaY / 2,
            message.width + deltaX,
            message.height + deltaY,
            SWP_FRAMECHANGED);
    }

    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();

    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(m_handle);
    ShowWindow(m_handle, SW_NORMAL);
    AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);
}

void Window::procMsgShowCursor(const MsgShowCursor& message)
{
    if (message.showCursor != m_showCursor)
    {
        ShowCursor(message.showCursor ? TRUE : FALSE);
        m_showCursor = message.showCursor;
    }
}

HWND Window::getHandle() const
{
    return m_handle;
}

void Window::processMessages() const
{
    if (m_handle != nullptr)
    {
        MSG msg;
        while (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
