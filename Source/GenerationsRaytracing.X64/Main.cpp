#include "ProcessUtil.h"
#include "RaytracingDevice.h"

static constexpr LPCTSTR s_gensProcessName = TEXT("SonicGenerations.exe");

static std::unique_ptr<RaytracingDevice> s_device;

static struct ThreadHolder
{
    std::thread processThread;

    ~ThreadHolder()
    {
        processThread.join();
    }
} s_threadHolder;

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
        return -1;

    const HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, *process);
    if (!processHandle)
        return -1;

    s_device = std::make_unique<RaytracingDevice>();

    s_threadHolder.processThread = std::thread([processHandle]
    {
        WaitForSingleObject(processHandle, INFINITE);
        s_device->setShouldExit();
    });

    s_device->runLoop();
    s_device->setEvents();

    return 0;
}
