#include "Bridge.h"
#include "MemoryMappedFile.h"
#include "Process.h"

int main()
{
#ifdef _DEBUG 
    if (GetConsoleWindow())
        freopen("CONOUT$", "w", stdout);

    if (!isProcessRunning(TEXT("SonicGenerations.exe")))
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

        while (!isProcessRunning(TEXT("SonicGenerations.exe")))
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

    Bridge bridge;
    bridge.receiveMessages();
}
