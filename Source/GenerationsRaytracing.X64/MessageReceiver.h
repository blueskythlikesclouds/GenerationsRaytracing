#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"
#include "MessageQueue.h"

class MessageReceiver
{
protected:
    Event m_x86Event{ Event::s_x86EventName };
    Event m_x64Event{ Event::s_x64EventName };

    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_memoryMap;

    uint32_t m_offset = sizeof(MessageQueue);

public:
    MessageReceiver();
    ~MessageReceiver();

    bool hasNext() const;
    uint8_t getId() const;

    template <typename T>
    const T& getMessage();

    void sync();
};

#include "MessageReceiver.inl"
