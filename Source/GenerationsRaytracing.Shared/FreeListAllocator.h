#pragma once

#include <vector>

#ifndef _WIN64
#include "Mutex.h"
#endif

class FreeListAllocator
{
protected:
    std::vector<uint32_t> m_indices;
    uint32_t m_capacity = 1; // Reserve first index for NULL
#ifndef _WIN64
    Mutex m_mutex;
#endif

public:
    uint32_t allocate();
    void free(uint32_t index);
};

#include "FreeListAllocator.inl"
