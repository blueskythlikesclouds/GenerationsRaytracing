#pragma once

class Mutex;

class LockGuard
{
protected:
    Mutex& m_mutex;

public:
    LockGuard(Mutex& mutex);
    ~LockGuard();

    LockGuard(LockGuard&&) = delete;
    LockGuard(const LockGuard&) = delete;
};

#include "LockGuard.inl"