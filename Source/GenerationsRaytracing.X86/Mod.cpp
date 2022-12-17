#include "Configuration.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"
#include "Patches.h"
#include "Process.h"

Mutex mutex;
MemoryMappedFile memoryMappedFile;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}

extern "C" __declspec(dllexport) void Init()
{
    if (!Configuration::load("GenerationsRaytracing.ini"))
        MessageBox(nullptr, TEXT("Unable to open \"GenerationsRaytracing.ini\" in mod directory."), TEXT("GenerationsRaytracing"), MB_ICONERROR);

#ifdef _DEBUG
    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);

    if (isProcessRunning(TEXT("GenerationsRaytracing.X64.exe")))
        return;

    MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
#endif

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation{};

    CreateProcess(
        TEXT("GenerationsRaytracing.X64.exe"),
        nullptr,
        nullptr,
        nullptr,
        TRUE,
#ifdef _DEBUG 
        0,
#else
        CREATE_NO_WINDOW,
#endif
        nullptr,
        nullptr,
        &startupInfo,
        &processInformation);
}

extern "C" __declspec(dllexport) void PostInit()
{
    Patches::init();
}

#if _DEBUG
extern "C" __declspec(dllexport) void OnFrame()
{
    if (!isProcessRunning(TEXT("GenerationsRaytracing.X64.exe")))
        exit(0);
}
#endif