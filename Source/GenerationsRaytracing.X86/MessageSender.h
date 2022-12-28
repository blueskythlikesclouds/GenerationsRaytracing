#pragma once

#include "CriticalSection.h"
#include "Event.h"
#include "MemoryMappedFile.h"

class Device;

struct MessageSender
{
    CriticalSection criticalSection;
    
    Event cpuEvent;
    Event gpuEvent;

    MemoryMappedFile memoryMappedFile;
    void* memoryMappedFileBuffer = nullptr;

    std::vector<uint8_t> buffer;

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

    template<typename T>
    void oneShot()
    {
        start<T>();
        finish();
    }

    void commitAllMessages();  
};

extern MessageSender msgSender;