#include "MessageReceiver.h"

MessageReceiver::MessageReceiver() : m_offset(0)
{
    m_messageBuffer = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageReceiver::~MessageReceiver()
{
    m_memoryMappedFile.unmap(m_messageBuffer);
}

uint8_t MessageReceiver::getId()
{
    uint8_t id = *(m_messageBuffer + m_offset);
    if ((id & 0x80) != 0)
    {
        m_offset += id & 0x7F;
        id = *(m_messageBuffer + m_offset);
    }
    return id;
}

void MessageReceiver::reset()
{
    m_offset = 0;
}
