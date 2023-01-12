#include "MessageReceiver.h"

#include "Message.h"

MessageReceiver::MessageReceiver()
    : cpuEvent(TEXT(EVENT_NAME_CPU)), gpuEvent(TEXT(EVENT_NAME_GPU))
{
    buffer = (uint8_t*)memoryMappedFile.map();
}

MessageReceiver::~MessageReceiver()
{
    memoryMappedFile.unmap(buffer);
}

bool MessageReceiver::hasNext()
{
    if (position == INVALID_POSITION_VALUE)
    {
        cpuEvent.wait();
        cpuEvent.reset();

        position = 0;
    }

    if (position < MEMORY_MAPPED_FILE_SIZE && *(int*)(buffer + position) != 0)
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

    position = INVALID_POSITION_VALUE;

    gpuEvent.set();
    return false;
}

int MessageReceiver::getMsgId() const
{
    return *(int*)(buffer + position);
}

void* MessageReceiver::getMsgAndMoveNext(size_t msgSize)
{
    void* command = buffer + position;
    position += MSG_ALIGN(msgSize);
    return command;
}

void* MessageReceiver::getDataAndMoveNext(size_t dataSize)
{
    void* data = buffer + position;
    position += MSG_ALIGN(dataSize);
    return data;
}
