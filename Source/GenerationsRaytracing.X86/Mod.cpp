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
    if (reason == DLL_PROCESS_DETACH)
        terminateProcess(GENERATIONS_RAYTRACING_X64);

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

    MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
#else
    terminateProcess(GENERATIONS_RAYTRACING_X64);
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