#pragma once

#include <vector>
#include "Mutex.h"

class SubAllocator
{
protected:
    std::vector<size_t> m_bitmap;
    uint32_t m_searchIndex = 0;
    Mutex m_mutex;

public:
    SubAllocator();

    uint32_t allocate();
    void free(uint32_t index);
};

#include "SubAllocator.inl"