#include "LockGuard.h"

inline uint32_t FreeListAllocator::allocate()
{
    LockGuard lock(m_mutex);

    uint32_t index;

    if (!m_indices.empty())
    {
        index = m_indices.back();
        m_indices.pop_back();
    }
    else
    {
        index = m_capacity;
        ++m_capacity;
    }

    return index;
}

inline void FreeListAllocator::free(uint32_t index)
{
    LockGuard lock(m_mutex);

    m_indices.push_back(index);
}

