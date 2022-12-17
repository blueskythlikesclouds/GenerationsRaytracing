#pragma once

#include "CriticalSection.h"
#include "Event.h"
#include "MemoryMappedFile.h"

struct MessageSender
{
    CriticalSection criticalSection;
    
    Event cpuEvent;
    Event gpuEvent;

    MemoryMappedFile memoryMappedFile;
    void* buffer = nullptr;
    size_t position = 0;

    std::atomic<size_t> messagesInProgress;

    MessageSender();
    ~MessageSender();

    void* start(size_t msgSize, size_t dataSize = 0);

    template<typename T>
    T* start(size_t dataSize = 0)
    {
        T* msg = (T*)start(sizeof(T), dataSize);
        msg->id = T::ID;
        return msg;
    }

    void finish();

    void commitAllMessages();  
};

extern MessageSender msgSender;