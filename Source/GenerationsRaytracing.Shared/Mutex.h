#pragma once

#include <Windows.h>
#include <cassert>

#define MUTEX_NAME "GenerationsRaytracingMutex"

struct Mutex
{
    HANDLE handle = nullptr;
#ifdef _DEBUG
    mutable bool locked = false;
#endif

    Mutex()
    {
#ifdef _WIN64
        handle = OpenMutex(
            MUTEX_ALL_ACCESS, 
            FALSE, 
            TEXT(MUTEX_NAME));

        static_assert(sizeof(size_t) == 8);
#else
        handle = CreateMutex(
            nullptr, 
            FALSE, 
            TEXT(MUTEX_NAME));

        static_assert(sizeof(size_t) == 4);
#endif
        assert(handle);
    }

    ~Mutex()
    {
        assert(!locked);
        const BOOL result = CloseHandle(handle);
        assert(result == TRUE);
    }

    void lock() const
    {
        assert(!locked);
        const DWORD result = WaitForSingleObject(handle, INFINITE);
        assert(result == WAIT_OBJECT_0);
#ifdef _DEBUG
        locked = true;
#endif
    }

    void unlock() const
    {
        assert(locked);
        const BOOL result = ReleaseMutex(handle);
        assert(result == TRUE);
#ifdef _DEBUG
        locked = false;
#endif
    }

    struct Lock
    {
        const Mutex& mutex;

        Lock(const Mutex& mutex)
            : mutex(mutex)
        {
            mutex.lock();
        }

        ~Lock()
        {
            mutex.unlock();
        }
    };
};