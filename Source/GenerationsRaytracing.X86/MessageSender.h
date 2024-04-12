#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"
#include "Mutex.h"

static size_t* s_shouldExit = reinterpret_cast<size_t*>(0x1E5E2E8);

class MessageSender
{
protected:
    Event m_cpuEvent{ Event::s_cpuEventName, FALSE };
    Event m_gpuEvent{ Event::s_gpuEventName, TRUE };
    Mutex m_mutex;
    uint8_t* m_messages = nullptr;
    uint32_t m_offset = sizeof(uint32_t);
    std::atomic<uint32_t> m_pendingMessages;
    MemoryMappedFile m_memoryMappedFile;
    uint8_t* m_memoryMap;

    std::chrono::high_resolution_clock::time_point m_time;
    double m_x86Duration{};
    double m_x64Duration{};
    uint32_t m_lastCommittedSize{};

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

    void commitMessages();

    void notifyShouldExit() const;

    double getX86Duration() const;
    double getX64Duration() const;
    uint32_t getLastCommittedSize() const;
};

inline MessageSender s_messageSender;

#include "MessageSender.inl"
