#pragma once

#include "MemoryMappedFile.h"

class MessageReceiver
{
protected:
    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_memoryMap;
    std::unique_ptr<uint8_t[]> m_messages;
    uint32_t m_offset = 0;
    uint32_t m_length = 0;
#ifdef _DEBUG
    std::vector<const char*> m_types;
#endif

public:
    MessageReceiver();
    ~MessageReceiver();

    bool hasNext() const;
    uint8_t getId() const;

    template <typename T>
    const T& getMessage();

    void receiveMessages();
};

#include "MessageReceiver.inl"
