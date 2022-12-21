#include "Configuration.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"
#include "Patches.h"
#include "Process.h"

constexpr LPCTSTR GENERATIONS_RAYTRACING_X64 = TEXT("GenerationsRaytracing.X64.exe");

Mutex mutex;
MemoryMappedFile memoryMappedFile;

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    return TRUE;
}

extern "C" __declspec(dllexport) void Init()
{
    std::atexit([] { terminateProcess(GENERATIONS_RAYTRACING_X64); });

    if (!Configuration::load("GenerationsRaytracing.ini"))
        MessageBox(nullptr, TEXT("Unable to open \"GenerationsRaytracing.ini\" in mod directory."), TEXT("GenerationsRaytracing"), MB_ICONERROR);

#ifdef _DEBUG
    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);

    if (findProcess(GENERATIONS_RAYTRACING_X64))
        return;

    MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
#endif

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInformation{};

    CreateProcess(
        GENERATIONS_RAYTRACING_X64,
        nullptr,
        nullptr,
        nullptr,
        TRUE,
#ifdef _DEBUG 
        0,
#else
        CREATE_NO_WINDOW | INHERIT_PARENT_AFFINITY,
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