#pragma once

#include <Windows.h>

struct Event
{
protected:
    HANDLE m_handle = nullptr;

public:
    static constexpr TCHAR s_x86EventName[] = TEXT("GenerationsRaytracingX86Event");
    static constexpr TCHAR s_x64EventName[] = TEXT("GenerationsRaytracingX64Event");
    static constexpr TCHAR s_swapChainEventName[] = TEXT("GenerationsRaytracingSwapChainEvent");

    Event(LPCTSTR name, BOOL manualReset, BOOL initialState);
    Event(LPCTSTR name);
    ~Event();

    void wait() const;
    bool waitImm() const;

    void set() const;
    void reset() const;
};

#include "Event.inl"