#include "MessageReceiver.h"
#include "MessageQueue.h"

MessageReceiver::MessageReceiver()
{
    m_memoryMap = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageReceiver::~MessageReceiver()
{
    m_memoryMappedFile.unmap(m_memoryMap);
}

bool MessageReceiver::hasNext() const
{
    return m_offset < MESSAGE_QUEUE->offset;
}

uint8_t MessageReceiver::getId() const
{
    return m_memoryMap[m_offset];
}

void MessageReceiver::sync()
{
    if (m_offset <= MESSAGE_QUEUE->offset && MESSAGE_QUEUE->sync)
    {
        m_x64Event.set();
        m_offset = sizeof(MessageQueue);
        m_x86Event.wait();
    }
}
