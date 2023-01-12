#include "Configuration.h"
#include "MemoryMappedFile.h"
#include "MessageSender.h"
#include "Patches.h"
#include "ProcessUtil.h"

constexpr LPCTSTR GENERATIONS_RAYTRACING_X64 = TEXT("GenerationsRaytracing.X64.exe");

static std::thread thread;

void setShouldExit()
{
    msgSender.cpuEvent.set();
    msgSender.gpuEvent.set();
    *(size_t*)0x1E5E2E8 = true;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}

extern "C" __declspec(dllexport) void Init()
{
    Patches::init();

    if (!Configuration::load("GenerationsRaytracing.ini"))
        MessageBox(nullptr, TEXT("Unable to open \"GenerationsRaytracing.ini\" in mod directory."), TEXT("GenerationsRaytracing"), MB_ICONERROR);

#ifdef _DEBUG
    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);

    if (findProcess(GENERATIONS_RAYTRACING_X64))
        return;

    //MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
#else
    terminateProcess(GENERATIONS_RAYTRACING_X64);
#endif

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation{};

    const BOOL result = CreateProcess(
        GENERATIONS_RAYTRACING_X64,
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

    if (!result)
        return setShouldExit();

    thread = std::thread([handle = processInformation.hProcess]
    {
        WaitForSingleObject(handle, INFINITE);
        setShouldExit();
    });

    std::atexit([] { thread.join(); });
}

#define ENSURE_DLL_NOT_LOADED(x) \
    if (GetModuleHandle(TEXT(x ".dll")) != nullptr) \
    { \
        MessageBox(nullptr, TEXT(x " must be disabled for this mod to function properly."), TEXT("GenerationsRaytracing"), MB_ICONERROR); \
        exit(-1); \
    }

extern "C" __declspec(dllexport) void PostInit()
{
    ENSURE_DLL_NOT_LOADED("BetterFxPipeline");
    ENSURE_DLL_NOT_LOADED("GenerationsD3D9Ex");
    ENSURE_DLL_NOT_LOADED("GenerationsD3D11");
}