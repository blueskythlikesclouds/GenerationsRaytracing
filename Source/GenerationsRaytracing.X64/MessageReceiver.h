#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"

#define INVALID_POSITION_VALUE (size_t)-1

struct MessageReceiver
{
    Event cpuEvent;
    Event gpuEvent;

    MemoryMappedFile memoryMappedFile;
    void* buffer = nullptr;
    size_t position = INVALID_POSITION_VALUE;

    MessageReceiver();
    ~MessageReceiver();

    bool hasNext();

    int getMsgId() const;
    void* getMsgAndMoveNext(size_t msgSize);
    void* getDataAndMoveNext(size_t dataSize);

    template<typename T>
    T* getMsgAndMoveNext()
    {
        return (T*)getMsgAndMoveNext(sizeof(T));
    }
};