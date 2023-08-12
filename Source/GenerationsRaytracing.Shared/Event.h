#pragma once

#include <Windows.h>

struct Event
{
protected:
    HANDLE m_handle = nullptr;

public:
    static constexpr TCHAR s_cpuEventName[] = TEXT("GenerationsRaytracingCPUEvent");
    static constexpr TCHAR s_gpuEventName[] = TEXT("GenerationsRaytracingGPUEvent");

    Event(LPCTSTR name, BOOL initialState);
    Event(LPCTSTR name);

    void wait() const;
    bool waitImm() const;

    void set() const;
    void reset() const;
};

#include "Event.inl"