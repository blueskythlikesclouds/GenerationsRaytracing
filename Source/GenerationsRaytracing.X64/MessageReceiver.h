#pragma once

#include "Event.h"
#include "MemoryMappedFile.h"

struct MessageReceiver
{
    Event cpuEvent;
    Event gpuEvent;

    MemoryMappedFile memoryMappedFile;
    void* buffer = nullptr;
    size_t position = 0;

    MessageReceiver();

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