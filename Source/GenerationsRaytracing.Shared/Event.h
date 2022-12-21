#pragma once

#include <Windows.h>

#define EVENT_NAME_CPU "GenerationsRaytracedEventCPU"
#define EVENT_NAME_GPU "GenerationsRaytracedEventGPU"

struct Event
{
    HANDLE handle = nullptr;

    Event(LPCTSTR name, BOOL initialState)
    {
        handle = CreateEvent(
            nullptr,
            TRUE,
            initialState,
            name);

        assert(handle);
    }

    Event(LPCTSTR name)
    {
        handle = OpenEvent(
            EVENT_ALL_ACCESS,
            FALSE,
            name);

        assert(handle);
    }

    void wait() const
    {
        const DWORD result = WaitForSingleObject(handle, INFINITE);
        assert(result == WAIT_OBJECT_0);
    }

    void set() const
    {
        const BOOL result = SetEvent(handle);
        assert(result == TRUE);
    }

    void reset() const
    {
        const BOOL result = ResetEvent(handle);
        assert(result == TRUE);
    }
};