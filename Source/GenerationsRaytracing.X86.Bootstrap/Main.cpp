#include <Windows.h>
#include <tlhelp32.h>

int main()
{
    const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    PROCESSENTRY32 processEntry{};
    processEntry.dwSize = sizeof(processEntry);

    HANDLE handles[2]{};
    size_t handleNum = 0;

    if (Process32First(hSnapshot, &processEntry))
    {
        while (Process32Next(hSnapshot, &processEntry))
        {
            if (wcsstr(processEntry.szExeFile, L"SonicGenerations.exe") != nullptr ||
                wcsstr(processEntry.szExeFile, L"GenerationsRaytracing.X64.exe") != nullptr)
            {
                handles[handleNum] = OpenProcess(SYNCHRONIZE, FALSE, processEntry.th32ProcessID);

                if (handles[handleNum])
                    ++handleNum;
            }

            if (handleNum == _countof(handles))
                break;
        }
    }

    CloseHandle(hSnapshot);

    if (handleNum > 0)
    {
        WaitForMultipleObjects(handleNum, handles, TRUE, 10 * 1000);

        for (size_t i = 0; i < handleNum; i++)
            CloseHandle(handles[i]);
    }

#if 0
    ShellExecute(
        nullptr,
        nullptr,
        TEXT("steam://run/71340"),
        nullptr,
        nullptr,
        NULL);
#else
    TCHAR currentDirectory[0x400]{};
    GetCurrentDirectory(_countof(currentDirectory), currentDirectory);

    STARTUPINFO startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInformation{};

    CreateProcess(
        TEXT("SonicGenerations.exe"),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        currentDirectory,
        &startupInfo,
        &processInformation);
#endif 

    return 0;
}