#pragma once

#include <Windows.h>
#include <tlhelp32.h>
#include <cassert>

static bool isProcessRunning(LPCTSTR name)
{
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    assert(handle);

    PROCESSENTRY32 processEntry{};
    processEntry.dwSize = sizeof(processEntry);

    bool found = false;

    if (Process32First(handle, &processEntry))
    {
        while (Process32Next(handle, &processEntry))
        {
            found |= wcsstr(processEntry.szExeFile, name) != nullptr;
            if (found)
                break;
        }
    }

    CloseHandle(handle);
    return found;
}