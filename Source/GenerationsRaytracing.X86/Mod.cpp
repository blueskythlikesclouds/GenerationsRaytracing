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
#include "RaytracingParams.h"
#include "ShaderCache.h"
#include "Sofdec.h"
#include "ToneMap.h"
#include "TriangleFan.h"
#include "TriangleStrip.h"
#include "Window.h"
#include "FileBinder.h"

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

extern "C" void __declspec(dllexport) Init(ModInfo_t* modInfo)
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
    FileBinder::init(modInfo);

#ifdef _DEBUG
#if 0
    MessageBox(NULL, TEXT("Attach to Process"), TEXT("Generations Raytracing"), MB_ICONINFORMATION | MB_OK);

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

#if 0

extern "C" void __declspec(dllexport) OnFrame()
{
    if (GetAsyncKeyState(VK_F1) & 0x1)
    {
        const auto gameDocument = Sonic::CGameDocument::GetInstance();
        if (gameDocument)
        {
            const auto lightManager = gameDocument->m_pMember->m_spLightManager;
            if (lightManager)
            {
                XXH64_hash_t hash = 0;
                hash = XXH64(lightManager->m_GlobalLightDirection.data(), 12, hash);
                hash = XXH64(lightManager->m_GlobalLightDiffuse.data(), 12, hash);

                char text[0x400];
                sprintf(text, "{ 0x%llx, { 0, 0, 0 } }, // %s", hash, gameDocument->m_pMember->m_StageName.c_str());

                if (OpenClipboard(nullptr))
                {
                    EmptyClipboard();
                    const size_t strLen = strlen(text) + 1;
                    const HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, strLen);
                    memcpy(GlobalLock(hGlobal), text, strLen);
                    SetClipboardData(CF_TEXT, hGlobal);
                    GlobalUnlock(hGlobal);
                    CloseClipboard();
                }
            }
        }
    }
    else if (GetAsyncKeyState(VK_F2) & 0x1)
    {
        *reinterpret_cast<bool*>(0x1A421FB) = false;
        *reinterpret_cast<bool*>(0x1A4358E) = false;
        *reinterpret_cast<bool*>(0x1A4323D) = false;
        *reinterpret_cast<bool*>(0x1A4358D) = false;
        *reinterpret_cast<bool*>(0x1E5E333) = false;
        *reinterpret_cast<bool*>(0x1B22E8C) = false;
        *reinterpret_cast<uint32_t*>(0x1E5E3E0) = 1;
        RaytracingParams::s_enable = false;
        RaytracingParams::s_toneMapMode = TONE_MAP_MODE_DISABLE;
    }
}

#endif