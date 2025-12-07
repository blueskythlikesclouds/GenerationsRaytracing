#pragma once

#include <Windows.h>

class Mutex : protected CRITICAL_SECTION
{
public:
    Mutex();
    ~Mutex();

    Mutex(Mutex&&) = delete;
    Mutex(const Mutex&) = delete;

    void lock();
    void unlock();

    bool tryLock();
};

#include "Mutex.inl"