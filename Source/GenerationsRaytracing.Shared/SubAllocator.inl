#include <cassert>
#include "LockGuard.h"

static constexpr size_t s_one = 1;
static constexpr uint32_t s_bits = sizeof(size_t) * 8;

inline SubAllocator::SubAllocator()
{
    // Reserve first index for NULL
    m_bitmap.push_back(1);
}

inline uint32_t SubAllocator::allocate()
{
    LockGuard lock(m_mutex);
    uint32_t index = 0;
    static_assert(sizeof(unsigned long) == sizeof(uint32_t));

    for (uint32_t i = m_searchIndex; i < m_bitmap.size(); i++)
    {
        if (!
#ifdef _WIN64
        _BitScanForward64
#else
        _BitScanForward
#endif
        (reinterpret_cast<unsigned long*>(&index), ~m_bitmap[i]))
        {
            continue;
        }

        m_bitmap[i] |= s_one << index;
        m_searchIndex = i;

        return i * s_bits + index;
    }

    m_searchIndex = static_cast<uint32_t>(m_bitmap.size());
    index = m_searchIndex * s_bits;
    m_bitmap.push_back(1);

    return index;
}

inline void SubAllocator::free(uint32_t index)
{
    LockGuard lock(m_mutex);

    assert((m_bitmap[index / s_bits] & (s_one << (index % s_bits))) != 0);

    m_bitmap[index / s_bits] &= ~(s_one << (index % s_bits));

    if (m_searchIndex > index)
        m_searchIndex = index;
}

