#pragma once

#include "MemoryMappedFile.h"

class MessageReceiver
{
protected:
    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_messageBuffer;
    size_t m_offset;

public:
    MessageReceiver();
    ~MessageReceiver();

    uint8_t getId();

    template <typename T>
    const T& getMessage();

    void reset();
};

#include "MessageReceiver.inl"
