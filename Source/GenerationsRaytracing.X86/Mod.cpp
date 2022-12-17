#include "Configuration.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"
#include "Patches.h"

Mutex mutex;
MemoryMappedFile memoryMappedFile;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}

extern "C" __declspec(dllexport) void Init()
{
#ifdef _DEBUG
    MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
#endif

    if (!Configuration::load("GenerationsRaytracing.ini"))
        MessageBox(nullptr, TEXT("Unable to open \"GenerationsRaytracing.ini\" in mod directory."), TEXT("GenerationsRaytracing"), MB_ICONERROR);

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation{};

    CreateProcess(
        TEXT("GenerationsRaytracing.X64.exe"),
        nullptr,
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW & 0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInformation);
}

extern "C" __declspec(dllexport) void PostInit()
{
    Patches::init();
}