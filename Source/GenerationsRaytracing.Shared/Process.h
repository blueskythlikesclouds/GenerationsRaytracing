#pragma once

#include <Windows.h>
#include <tlhelp32.h>
#include <cassert>
#include <optional>

static auto findProcess(const LPCTSTR name)
{
    HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    assert(handle);

    PROCESSENTRY32 processEntry{};
    processEntry.dwSize = sizeof(processEntry);

    bool foundProcess = false;

    if (Process32First(handle, &processEntry))
    {
        while (Process32Next(handle, &processEntry))
        {
            if (wcsstr(processEntry.szExeFile, name) != nullptr)
            {
                foundProcess = true;
                break;
            }
        }
    }

    CloseHandle(handle);

    return foundProcess ? std::optional(processEntry.th32ProcessID) : std::nullopt;
}

static void terminateProcess(const LPCTSTR name)
{
    auto process = findProcess(name);
    if (!process)
        return;

    const HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, *process);
    if (!handle)
        return;

    TerminateProcess(handle, 1);
    CloseHandle(handle);
}