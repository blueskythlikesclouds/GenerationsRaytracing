#include "ModelData.h"
#include "Configuration.h"
#include "D3D9.h"
#include "PictureData.h"
#include "FillTexture.h"
#include "HalfPixel.h"
#include "MaterialData.h"
#include "MessageSender.h"
#include "ProcessUtil.h"
#include "RaytracingRendering.h"
#include "InstanceData.h"
#include "MemoryAllocator.h"
#include "MeshData.h"
#include "MotionData.h"
#include "ShaderCache.h"
#include "Sofdec.h"
#include "ToneMap.h"
#include "TriangleFan.h"
#include "TriangleStrip.h"
#include "Window.h"

static constexpr LPCTSTR s_bridgeProcessName = TEXT("GenerationsRaytracing.X64.exe");

static struct ThreadHolder
{
    std::thread processThread;

    ~ThreadHolder()
    {
        if (processThread.joinable())
            processThread.join();
    }
} s_threadHolder;

static void setShouldExit()
{
    *s_shouldExit = true;
    s_messageSender.notifyShouldExit();
}

extern "C" void __declspec(dllexport) FilterMod(FilterModArguments_t& args)
{
    args.handled = strcmp(args.mod->Name, "Direct3D 9 Ex") == 0 || strcmp(args.mod->ID, "bsthlc.generationsd3d11") == 0;
}

extern "C" void __declspec(dllexport) PreInit()
{
    RaytracingRendering::preInit();
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
    ModelData::init();
    InstanceData::init();
    RaytracingRendering::init();
    MaterialData::init();
    Sofdec::init();
    ShaderCache::init();
    MotionData::init();
    MemoryAllocator::init();
    MeshData::init();
    ToneMap::init();

#ifdef _DEBUG
#if 0
    MessageBox(NULL, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_ICONINFORMATION | MB_OK);

    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);
#endif
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

extern "C" void __declspec(dllexport) PostInit()
{
    RaytracingRendering::postInit();
    MaterialData::postInit();
}