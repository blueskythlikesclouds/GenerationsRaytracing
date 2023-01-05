#include "Configuration.h"
#include "MemoryMappedFile.h"
#include "Patches.h"
#include "Process.h"

constexpr LPCTSTR GENERATIONS_RAYTRACING_X64 = TEXT("GenerationsRaytracing.X64.exe");

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

    //MessageBox(nullptr, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_OK);
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

#define ENSURE_DLL_NOT_LOADED(x) \
    if (GetModuleHandle(TEXT(x ".dll")) != nullptr) \
    { \
        MessageBox(nullptr, TEXT(x " must be disabled for this mod to function properly."), TEXT("GenerationsRaytracing"), MB_ICONERROR); \
        terminateProcess(GENERATIONS_RAYTRACING_X64); \
        exit(-1); \
    }

extern "C" __declspec(dllexport) void PostInit()
{
    ENSURE_DLL_NOT_LOADED("BetterFxPipeline");
    ENSURE_DLL_NOT_LOADED("GenerationsD3D9Ex");
    ENSURE_DLL_NOT_LOADED("GenerationsD3D11");
    ENSURE_DLL_NOT_LOADED("GenerationsParameterEditor");
}