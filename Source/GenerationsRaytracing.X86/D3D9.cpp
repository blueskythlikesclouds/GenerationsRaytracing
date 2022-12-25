// Copy pasted from GenerationsD3D11:
// https://github.com/blueskythlikesclouds/DllMods/blob/master/Source/GenerationsD3D11/Mod/D3D9.cpp

#include "D3D9.h"

#include "Configuration.h"
#include "Device.h"
#include "Message.h"
#include "MessageSender.h"
#include "Surface.h"
#include "Texture.h"

void D3D9::ensureNotNull()
{
    if (!d3d9)
        d3d9.Attach(Direct3DCreate9(D3D_SDK_VERSION));
}

FUNCTION_STUB(HRESULT, D3D9::RegisterSoftwareDevice, void* pInitializeFunction)

UINT D3D9::GetAdapterCount()
{
    ensureNotNull();
    return d3d9->GetAdapterCount();
}

HRESULT D3D9::GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier)
{
    ensureNotNull();
    return d3d9->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
}

UINT D3D9::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format)
{
    ensureNotNull();
    return d3d9->GetAdapterModeCount(Adapter, Format);
}

HRESULT D3D9::EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode)
{
    ensureNotNull();
    return d3d9->EnumAdapterModes(Adapter, Format, Mode, pMode);
}

HRESULT D3D9::GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode)
{
    ensureNotNull();
    return d3d9->GetAdapterDisplayMode(Adapter, pMode);
}

FUNCTION_STUB(HRESULT, D3D9::CheckDeviceType, UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed)

HRESULT D3D9::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
    ensureNotNull();
    return d3d9->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);
}

FUNCTION_STUB(HRESULT, D3D9::CheckDeviceMultiSampleType, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels)

FUNCTION_STUB(HRESULT, D3D9::CheckDepthStencilMatch, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)

FUNCTION_STUB(HRESULT, D3D9::CheckDeviceFormatConversion, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat)

HRESULT D3D9::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps)
{
    ensureNotNull();
    return d3d9->GetDeviceCaps(Adapter, DeviceType, pCaps);
}

FUNCTION_STUB(HMONITOR, D3D9::GetAdapterMonitor, UINT Adapter)

HRESULT D3D9::CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, Device** ppReturnedDeviceInterface)
{
    DisplayMode displayMode = Configuration::displayMode;
    if (displayMode != DisplayMode::Windowed && *(uint8_t*)0xA5EB5B != 0x89) // Check if Borderless Fullscreen patch is enabled in HMM
        displayMode = DisplayMode::BorderlessFullscreen;

    LONG_PTR windowStyle = WS_POPUP;

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);

    ensureNotNull();
    GetMonitorInfo(d3d9->GetAdapterMonitor(Adapter), &monitorInfo);
    d3d9 = nullptr;

    uint32_t x, y, width, height;

    if (displayMode == DisplayMode::Borderless || displayMode == DisplayMode::Windowed)
    {
        width = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
        height = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

        x = monitorInfo.rcWork.left;
        y = monitorInfo.rcWork.top;

        if (pPresentationParameters->BackBufferWidth > width)
        {
            width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
            x = monitorInfo.rcMonitor.left;
        }

        if (pPresentationParameters->BackBufferHeight > height)
        {
            height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
            y = monitorInfo.rcMonitor.top;
        }

        x += (width - pPresentationParameters->BackBufferWidth) / 2;
        y += (height - pPresentationParameters->BackBufferHeight) / 2;

        width = pPresentationParameters->BackBufferWidth;
        height = pPresentationParameters->BackBufferHeight;

        if (displayMode == DisplayMode::Windowed)
        {
            windowStyle = WS_OVERLAPPEDWINDOW;
            if (!Configuration::allowResizeInWindowed)
                windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }
    }
    else
    {
        x = monitorInfo.rcMonitor.left;
        y = monitorInfo.rcMonitor.top;
        width = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    }

    if (displayMode == DisplayMode::Windowed)
        ShowCursor(true);

    *ppReturnedDeviceInterface = new Device(pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight);

    const auto msg = msgSender.start<MsgInitSwapChain>();

    msg->width = pPresentationParameters->BackBufferWidth;
    msg->height = pPresentationParameters->BackBufferHeight;
    msg->bufferCount = pPresentationParameters->BackBufferCount;
    msg->scaling = Configuration::allowResizeInWindowed ||
        width != pPresentationParameters->BackBufferWidth ||
        height != pPresentationParameters->BackBufferHeight ? DXGI_SCALING_STRETCH : DXGI_SCALING_NONE;
    msg->handle = (unsigned int)pPresentationParameters->hDeviceWindow;
    msg->surface = (*ppReturnedDeviceInterface)->swapChainSurface->texture->id;

    msgSender.finish();

    // Force the window to the foreground.
    const DWORD windowThreadProcessId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentThreadId = GetCurrentThreadId();
    AttachThreadInput(windowThreadProcessId, currentThreadId, TRUE);
    BringWindowToTop(pPresentationParameters->hDeviceWindow);
    ShowWindow(pPresentationParameters->hDeviceWindow, SW_SHOW);
    AttachThreadInput(windowThreadProcessId, currentThreadId, FALSE);

    SetWindowLongPtr(pPresentationParameters->hDeviceWindow, GWL_STYLE, WS_VISIBLE | windowStyle);
    SetWindowPos(pPresentationParameters->hDeviceWindow, HWND_TOP, x, y, width, height, SWP_FRAMECHANGED);

    // In windowed, title bar and border are included when setting width/height. Let's fix that and not be like Lost World/Forces.
    if (displayMode == DisplayMode::Windowed)
    {
        RECT windowRect, clientRect;
        GetWindowRect(pPresentationParameters->hDeviceWindow, &windowRect);
        GetClientRect(pPresentationParameters->hDeviceWindow, &clientRect);

        uint32_t windowWidth = windowRect.right - windowRect.left;
        uint32_t windowHeight = windowRect.bottom - windowRect.top;

        uint32_t clientWidth = clientRect.right - clientRect.left;
        uint32_t clientHeight = clientRect.bottom - clientRect.top;

        uint32_t deltaX = windowWidth - clientWidth;
        uint32_t deltaY = windowHeight - clientHeight;

        width += deltaX;
        height += deltaY;

        x -= deltaX / 2;
        y -= deltaY / 2;

        SetWindowPos(pPresentationParameters->hDeviceWindow, HWND_TOP, x, y, width, height, SWP_FRAMECHANGED);
    }

    return S_OK;
}