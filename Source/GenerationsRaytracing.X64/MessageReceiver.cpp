#include "MessageReceiver.h"

MessageReceiver::MessageReceiver()
{
    m_memoryMap = static_cast<uint8_t*>(m_memoryMappedFile.map());
    m_messages = std::make_unique<uint8_t[]>(MemoryMappedFile::s_size);
}

MessageReceiver::~MessageReceiver()
{
    m_memoryMappedFile.unmap(m_memoryMap);
}

bool MessageReceiver::hasNext() const
{
    return m_offset < m_length;
}

uint8_t MessageReceiver::getId() const
{
    return m_messages[m_offset];
}

void MessageReceiver::receiveMessages()
{
    m_offset = sizeof(uint32_t);
    m_length = *reinterpret_cast<uint32_t*>(m_memoryMap);
    memcpy(m_messages.get(), m_memoryMap, m_length);
}
