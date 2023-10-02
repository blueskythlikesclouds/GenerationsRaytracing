#pragma once

#include <vector>
#include "Mutex.h"

class FreeListAllocator
{
protected:
    std::vector<uint32_t> m_indices;
    uint32_t m_capacity = 1; // Reserve first index for NULL
    Mutex m_mutex;

public:
    uint32_t allocate();
    void free(uint32_t index);
};

#include "FreeListAllocator.inl"
