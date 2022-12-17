#include "MessageSender.h"

#include "Message.h"

MessageSender::MessageSender()
    : cpuEvent(TEXT(EVENT_NAME_CPU), FALSE), gpuEvent(TEXT(EVENT_NAME_GPU), TRUE)
{
}

MessageSender::~MessageSender()
{
    start<MsgExit>();
    finish();
    commitAllMessages();
}

void* MessageSender::start(size_t msgSize, size_t dataSize)
{
    std::lock_guard lock(criticalSection);

    msgSize = MSG_ALIGN(msgSize);
    dataSize = MSG_ALIGN(dataSize);

    const size_t size = msgSize + dataSize;
    
    assert(size <= MEMORY_MAPPED_FILE_SIZE);

    if (position + size > MEMORY_MAPPED_FILE_SIZE)
        commitAllMessages();

    ++messagesInProgress;

    if (!buffer)
    {
        gpuEvent.wait();

        buffer = memoryMappedFile.map();
        position = 0;
    }

    void* msg = (char*)buffer + position;
    position += size;
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

    if (!buffer)
        return;

    while (messagesInProgress)
        ;

    assert((position & (MSG_ALIGNMENT - 1)) == 0);

    if (position < MEMORY_MAPPED_FILE_SIZE)
        *(int*)((char*)buffer + position) = 0;

    memoryMappedFile.unmap(buffer);
    buffer = nullptr;
    position = 0;

    cpuEvent.set();
    gpuEvent.reset();
}

MessageSender msgSender;