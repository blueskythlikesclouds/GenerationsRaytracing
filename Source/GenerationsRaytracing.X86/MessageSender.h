#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"

class MessageSender
{
protected:
    Event m_cpuEvent{ Event::s_cpuEventName, FALSE };
    Event m_gpuEvent{ Event::s_gpuEventName, TRUE };

    std::unique_ptr<uint8_t[]> m_parallelBuffer;
    uint32_t m_parallelBufferSize = 0;

    std::unique_ptr<uint8_t[]> m_serialBuffer;
    uint32_t m_serialBufferSize = 0;

    Mutex m_mutex;

    std::atomic<uint32_t> m_pendingMessages;

    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_tgtBufferData;

#ifdef _DEBUG
    std::atomic<uint32_t> m_serialCounter;
#endif

public:
    MessageSender();
    ~MessageSender();

    void* makeParallelMessage(uint32_t byteSize, uint32_t alignment);
    void endParallelMessage();

    void* makeSerialMessage(uint32_t byteSize, uint32_t alignment);

    void sendAllMessages();

    void setEvents() const;

    template<typename T>
    T& makeParallelMessage();

    template<typename T>
    T& makeParallelMessage(uint32_t dataSize);

    template<typename T>
    T& makeSerialMessage();

    template<typename T>
    T& makeSerialMessage(uint32_t dataSize);
};

inline MessageSender s_messageSender;

#include "MessageSender.inl"
