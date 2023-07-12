#include "Mutex.h"

inline LockGuard::LockGuard(Mutex& mutex) : m_mutex(mutex)
{
    mutex.lock();
}

inline LockGuard::~LockGuard()
{
    m_mutex.unlock();
}