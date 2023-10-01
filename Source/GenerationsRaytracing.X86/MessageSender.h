#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"

class MessageSender
{
protected:
    Event m_cpuEvent{ Event::s_cpuEventName, FALSE };
    Event m_gpuEvent{ Event::s_gpuEventName, TRUE };
    Mutex m_mutex;
    std::unique_ptr<uint8_t[]> m_messages;
    uint32_t m_offset = 0;
    std::atomic<uint32_t> m_pendingMessages;
    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_memoryMap;

public:
    MessageSender();
    ~MessageSender();

    void* makeMessage(uint32_t byteSize, uint32_t alignment);
    void endMessage();

    template<typename T>
    T& makeMessage();

    template<typename T>
    T& makeMessage(uint32_t dataSize);

    void commitMessages();

    void notifyShouldExit() const;
};

inline MessageSender s_messageSender;

#include "MessageSender.inl"
