#include "BottomLevelAccelStruct.h"
#include "Configuration.h"
#include "D3D9.h"
#include "PictureData.h"
#include "FillTexture.h"
#include "HalfPixel.h"
#include "MessageSender.h"
#include "ProcessUtil.h"
#include "Sky.h"
#include "TriangleFan.h"
#include "TriangleStrip.h"
#include "Window.h"

static constexpr LPCTSTR s_bridgeProcessName = TEXT("GenerationsRaytracing.X64.exe");
static size_t* s_shouldExit = reinterpret_cast<size_t*>(0x1E5E2E8);

static struct ThreadHolder
{
    std::thread processThread;

    ~ThreadHolder()
    {
        processThread.join();
    }
} s_threadHolder;

static void setShouldExit()
{
    s_messageSender.notifyShouldExit();
    *s_shouldExit = true;
}

extern "C" void __declspec(dllexport) Init()
{
    Configuration::init();
    D3D9::init();
    PictureData::init();
    FillTexture::init();
    Window::init();
    TriangleFan::init();
    HalfPixel::init();
    TriangleStrip::init();
    Sky::init();
    BottomLevelAccelStruct::init();

#ifdef _DEBUG
    MessageBox(NULL, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_ICONINFORMATION | MB_OK);

    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);
#else
    terminateProcess(s_bridgeProcessName);
#endif

    const auto process = findProcess(s_bridgeProcessName);
    HANDLE processHandle = nullptr;

    if (!process)
    {
        STARTUPINFO startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInformation{};

        const BOOL result = CreateProcess(
            s_bridgeProcessName,
            nullptr,
            nullptr,
            nullptr,
            FALSE,
#ifdef _DEBUG 
            0,
#else
            CREATE_NO_WINDOW | INHERIT_PARENT_AFFINITY,
#endif
            nullptr,
            nullptr,
            &startupInfo,
            &processInformation);

        assert(result == TRUE);

        processHandle = processInformation.hProcess;
    }
    else
    {
        processHandle = OpenProcess(SYNCHRONIZE, FALSE, *process);
    }

    assert(processHandle != nullptr);

    s_threadHolder.processThread = std::thread([processHandle]
    {
        WaitForSingleObject(processHandle, INFINITE);
        setShouldExit();
        CloseHandle(processHandle);
    });
}
