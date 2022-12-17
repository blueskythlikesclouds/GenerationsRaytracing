#pragma once

class CriticalSection
{
public:
    CRITICAL_SECTION criticalSection;

    CriticalSection()
    {
        InitializeCriticalSection(&criticalSection);
    }

    ~CriticalSection()
    {
        DeleteCriticalSection(&criticalSection);
    }

    void lock()
    {
        EnterCriticalSection(&criticalSection);
    }

    void unlock()
    {
        LeaveCriticalSection(&criticalSection);
    }
};