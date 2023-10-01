#include "MessageReceiver.h"

MessageReceiver::MessageReceiver()
{
    m_messageBuffer = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageReceiver::~MessageReceiver()
{
    m_memoryMappedFile.unmap(m_messageBuffer);
}

uint8_t MessageReceiver::getId()
{
    return m_messageBuffer[m_offset];
}

void MessageReceiver::reset()
{
    m_offset = 0;
}
