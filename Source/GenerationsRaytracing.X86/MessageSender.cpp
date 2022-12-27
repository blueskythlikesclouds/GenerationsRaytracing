#include "MessageSender.h"

#include "Device.h"
#include "Message.h"

MessageSender::MessageSender()
    : cpuEvent(TEXT(EVENT_NAME_CPU), FALSE), gpuEvent(TEXT(EVENT_NAME_GPU), TRUE)
{
    memoryMappedFileBuffer = memoryMappedFile.map();
}

MessageSender::~MessageSender()
{
    oneShot<MsgExit>();
    commitAllMessages();

    memoryMappedFile.unmap(memoryMappedFileBuffer);
}

void* MessageSender::start(size_t msgSize, size_t dataSize)
{
    std::lock_guard lock(criticalSection);

    msgSize = MSG_ALIGN(msgSize);
    dataSize = MSG_ALIGN(dataSize);

    const size_t size = msgSize + dataSize;
    
    assert(size <= MEMORY_MAPPED_FILE_SIZE);

    if (buffer.size() + size > MEMORY_MAPPED_FILE_SIZE)
        commitAllMessages();

    if (buffer.size() + size > buffer.capacity())
    {
        while (messagesInProgress)
            ;
    }

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

    assert((buffer.size() & (MSG_ALIGNMENT - 1)) == 0);

    if (buffer.size() <= MEMORY_MAPPED_FILE_SIZE - MSG_ALIGNMENT)
        buffer.resize(buffer.size() + MSG_ALIGNMENT);

    gpuEvent.wait();
    memcpy(memoryMappedFileBuffer, buffer.data(), buffer.size());
    cpuEvent.set();

    buffer.clear();

    if (device) device->reset();
}

MessageSender msgSender;