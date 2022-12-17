#include "MessageReceiver.h"

#include "Message.h"

MessageReceiver::MessageReceiver()
    : cpuEvent(TEXT(EVENT_NAME_CPU)), gpuEvent(TEXT(EVENT_NAME_GPU))
{
}

bool MessageReceiver::hasNext()
{
    if (!buffer)
    {
        cpuEvent.wait();
        buffer = memoryMappedFile.map();
        position = 0;
    }

    if (position < MEMORY_MAPPED_FILE_SIZE && *(int*)((char*)buffer + position) != 0)
        return true;

#if 0
    static int msgIdx = 0;
    char path[0x400];
    sprintf(path, "dbgmsg/%d.bin", msgIdx++);
    FILE* file = fopen(path, "wb");
    if (file)
    {
        fwrite(buffer, position, 1, file);
        fclose(file);
    }
#endif

    memoryMappedFile.unmap(buffer);

    buffer = nullptr;
    position = 0;

    gpuEvent.set();
    cpuEvent.reset();

    return false;
}

int MessageReceiver::getMsgId() const
{
    return *(int*)((char*)buffer + position);
}

void* MessageReceiver::getMsgAndMoveNext(size_t msgSize)
{
    void* command = (char*)buffer + position;
    position += MSG_ALIGN(msgSize);
    return command;
}

void* MessageReceiver::getDataAndMoveNext(size_t dataSize)
{
    void* data = (char*)buffer + position;
    position += MSG_ALIGN(dataSize);
    return data;
}
