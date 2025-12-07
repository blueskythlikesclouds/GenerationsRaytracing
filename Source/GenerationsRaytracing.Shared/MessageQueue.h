#pragma once

#include <cstdint>
#include <atomic>

struct MessageQueue
{
    std::atomic<uint32_t> offset;
    std::atomic<bool> sync;
};

#define MESSAGE_QUEUE reinterpret_cast<MessageQueue*>(m_memoryMap)