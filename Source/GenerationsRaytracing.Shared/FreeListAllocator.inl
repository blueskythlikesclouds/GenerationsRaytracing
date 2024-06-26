#include "LockGuard.h"
#include <cassert>

inline uint32_t FreeListAllocator::allocate()
{
#ifndef _WIN64
    LockGuard lock(m_mutex);
#endif

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
    assert(index != NULL);
#ifndef _WIN64
    LockGuard lock(m_mutex);
#endif
    m_indices.insert(std::lower_bound(m_indices.begin(), m_indices.end(), index, std::greater<uint32_t>()), index);
}

