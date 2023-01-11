#include "Bridge.h"
#include "Path.h"
#include "ProcessUtil.h"

constexpr LPCTSTR SONIC_GENERATIONS = TEXT("SonicGenerations.exe");

int main(int argc, char* argv[])
{
#ifdef _DEBUG 
    if (GetConsoleWindow())
        freopen("CONOUT$", "w", stdout);

    if (!findProcess(SONIC_GENERATIONS))
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

        while (!findProcess(SONIC_GENERATIONS))
            Sleep(10);

        printf("Waiting for memory mapped file...\n");

        HANDLE handle;
        do
        {
            handle = OpenFileMapping(FILE_MAP_READ, FALSE, TEXT(MEMORY_MAPPED_FILE_NAME));
            Sleep(10);
        } while (!handle);

        CloseHandle(handle);

        printf("Success!\n");
    }
#endif

    const auto process = findProcess(SONIC_GENERATIONS);
    if (!process)
        return -1;

    const HANDLE handle = OpenProcess(SYNCHRONIZE, FALSE, *process);
    if (!handle)
        return -1;

    const auto bridge = std::make_unique<Bridge>(getDirectoryPath(argv[0]));

    std::thread thread([&]
    {
        WaitForSingleObject(handle, INFINITE);

        bridge->msgReceiver.cpuEvent.set();
        bridge->msgReceiver.gpuEvent.set();
        bridge->shouldExit = true;
    });

    bridge->receiveMessages();

    thread.join();
    CloseHandle(handle);

    return 0;
}
