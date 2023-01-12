#include "MessageSender.h"

#include "Device.h"
#include "Message.h"

MessageSender::MessageSender()
    : cpuEvent(TEXT(EVENT_NAME_CPU), FALSE), gpuEvent(TEXT(EVENT_NAME_GPU), TRUE)
{
    memoryMappedFileBuffer = memoryMappedFile.map();
    buffer.reserve(MEMORY_MAPPED_FILE_SIZE);
}

MessageSender::~MessageSender()
{
    memoryMappedFile.unmap(memoryMappedFileBuffer);
}

void* MessageSender::start(size_t msgSize, size_t dataSize)
{
    msgSize = MSG_ALIGN(msgSize);
    dataSize = MSG_ALIGN(dataSize);

    const size_t size = msgSize + dataSize;
    
    assert(size <= MEMORY_MAPPED_FILE_SIZE);

    std::lock_guard lock(criticalSection);

    if (buffer.size() + size > MEMORY_MAPPED_FILE_SIZE)
        commitAllMessages();

    const size_t position = buffer.size();
    buffer.resize(buffer.size() + size);

    ++messagesInProgress;

    void* msg = buffer.data() + position;
    assert((position & (MSG_ALIGNMENT - 1)) == 0);
    return msg;
}

void MessageSender::finish()
{
    --messagesInProgress;
}

void MessageSender::commitAllMessages()
{
    std::lock_guard lock(criticalSection);

    if (buffer.empty())
        return;

    while (messagesInProgress)
        ;

    if (!*(size_t*)0x1E5E2E8)
    {
        assert((buffer.size() & (MSG_ALIGNMENT - 1)) == 0);

        if (buffer.size() <= MEMORY_MAPPED_FILE_SIZE - MSG_ALIGNMENT)
            buffer.resize(buffer.size() + MSG_ALIGNMENT);

        gpuEvent.wait();
        gpuEvent.reset();

        memcpy(memoryMappedFileBuffer, buffer.data(), buffer.size());
        cpuEvent.set();
    }

    buffer.clear();
}

MessageSender msgSender;
