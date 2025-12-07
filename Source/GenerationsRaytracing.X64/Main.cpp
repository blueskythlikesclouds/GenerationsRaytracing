#include "ProcessUtil.h"
#include "RaytracingDevice.h"

// Required so DLSS objects don't get destructed
// before we can call the shutdown function
#pragma init_seg(user)

static constexpr LPCTSTR s_gensProcessName = TEXT("SonicGenerations.exe");

static std::unique_ptr<Device> s_device;

static struct ThreadHolder
{
    std::thread processThread;
    HANDLE event;

    ThreadHolder()
        : event(CreateEvent(nullptr, FALSE, FALSE, nullptr))
    {
    }

    ~ThreadHolder()
    {
        SetEvent(event);

        if (processThread.joinable())
            processThread.join();

        CloseHandle(event);
    }
} s_threadHolder;

static void onProcessNotFound()
{
#ifndef _DEBUG
    MessageBox(nullptr,
        TEXT("This executable file cannot run independently. "
             "Please install and play Generations Raytracing using HedgeModManager, just like any other Sonic Generations mod."),
        TEXT("Generations Raytracing"),
        MB_ICONERROR);
#endif
}

int main()
{
#ifdef _DEBUG 
    if (GetConsoleWindow())
        freopen("CONOUT$", "w", stdout);

    if (!findProcess(s_gensProcessName))
    {
        STARTUPINFO startupInfo{};
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInformation{};

        CreateProcess(
            TEXT("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Sonic Generations\\SonicGenerations.exe"),
            nullptr,
            nullptr,
            nullptr,
            TRUE,
            0,
            nullptr,
            TEXT("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Sonic Generations"),
            &startupInfo,
            &processInformation);

        printf("Waiting for Sonic Generations...\n");

        while (!findProcess(s_gensProcessName))
            Sleep(10);

        printf("Waiting for memory mapped file...\n");

        HANDLE handle;
        do
        {
            handle = OpenFileMapping(FILE_MAP_READ, FALSE, MemoryMappedFile::s_name);
            Sleep(10);
        } while (!handle);

        CloseHandle(handle);

        printf("Success!\n");
    }
#endif

    const auto process = findProcess(s_gensProcessName);
    if (!process)
    {
        onProcessNotFound();
        return -1;
    }

    const HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, *process);
    if (!processHandle)
    {
        onProcessNotFound();
        return -1;
    }
    
    IniFile iniFile;
    iniFile.read("GenerationsRaytracing.ini");

    s_device = iniFile.getBool("Mod", "EnableRaytracing", true) ?
        std::make_unique<RaytracingDevice>(iniFile) : 
        std::make_unique<Device>(iniFile);

    if (s_device->getUnderlyingDevice() != nullptr)
    {
        s_threadHolder.processThread = std::thread([processHandle]
        {
            const HANDLE handles[] = { s_threadHolder.event, processHandle };
            DWORD waitResult = WaitForMultipleObjects(_countof(handles), handles, FALSE, INFINITE);
            CloseHandle(processHandle);

            if (waitResult == (WAIT_OBJECT_0 + 1))
                std::_Exit(0);
        });

        s_device->runLoop();
    }
    s_device.reset();

    return 0;
}
