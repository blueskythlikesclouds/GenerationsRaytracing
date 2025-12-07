#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"
#include "MessageQueue.h"
#include "Mutex.h"

static size_t* s_shouldExit = reinterpret_cast<size_t*>(0x1E5E2E8);

class MessageSender
{
protected:
    Event m_x86Event{ Event::s_x86EventName, FALSE, FALSE };
    Event m_x64Event{ Event::s_x64EventName, FALSE, FALSE };

    Mutex m_mutex;

    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_memoryMap;
    uint32_t m_offset = sizeof(MessageQueue);

public:
    static bool canMakeMessage(uint32_t byteSize, uint32_t alignment);

    template<typename T>
    static bool canMakeMessage(uint32_t dataSize);

    MessageSender();
    ~MessageSender();

    void* makeMessage(uint32_t byteSize, uint32_t alignment);
    void endMessage();

    template<typename T>
    T& makeMessage();

    template<typename T>
    T& makeMessage(uint32_t dataSize);

    template<typename T>
    void oneShotMessage();

    void sync();

    void notifyShouldExit();
};

inline MessageSender s_messageSender;

#include "MessageSender.inl"
