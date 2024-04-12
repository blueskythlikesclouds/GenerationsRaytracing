#include "MessageSender.h"

#include "LockGuard.h"
#include "Message.h"

bool MessageSender::canMakeMessage(uint32_t byteSize, uint32_t alignment)
{
    return ((sizeof(uint32_t) + alignment - 1) & ~(alignment - 1)) + byteSize <= MemoryMappedFile::s_size;
}

MessageSender::MessageSender() : m_pendingMessages(0)
{
    m_messages = static_cast<uint8_t*>(_aligned_malloc(MemoryMappedFile::s_size, 0x10));
    m_memoryMap = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageSender::~MessageSender()
{
    m_memoryMappedFile.unmap(m_memoryMap);
    _aligned_free(m_messages);
}

void* MessageSender::makeMessage(uint32_t byteSize, uint32_t alignment)
{
    assert(byteSize <= MemoryMappedFile::s_size);

    LockGuard lock(m_mutex);

    uint32_t alignedOffset = m_offset;

    if ((alignedOffset & (alignment - 1)) != 0)
    {
        alignedOffset += offsetof(MsgPadding, data);
        alignedOffset += alignment - 1;
        alignedOffset &= ~(alignment - 1);
    }
    if (alignedOffset + byteSize > (MemoryMappedFile::s_size - sizeof(uint32_t)))
    {
        commitMessages();
        alignedOffset = std::max(sizeof(uint32_t), alignment);
    }

    if (m_offset != alignedOffset)
    {
        const auto padding = reinterpret_cast<MsgPadding*>(&m_messages[m_offset]);
        padding->id = MsgPadding::s_id;
        padding->dataSize = alignedOffset - (m_offset + offsetof(MsgPadding, data));

#ifdef _DEBUG
        memset(padding->data, 0xCC, padding->dataSize);
#endif
    }

    void* message = &m_messages[alignedOffset];
#ifdef _DEBUG
    memset(message, 0xCC, byteSize);
#endif
    m_offset = alignedOffset + byteSize;
    ++m_pendingMessages;
    return message;
}

void MessageSender::endMessage()
{
    --m_pendingMessages;
}

static double computeDuration(const std::chrono::high_resolution_clock::time_point& time)
{
    return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
        std::chrono::high_resolution_clock::now() - time).count();
}

void MessageSender::commitMessages()
{
    LockGuard lock(m_mutex);

    while (m_pendingMessages)
        ;

    *reinterpret_cast<uint32_t*>(m_messages) = m_offset;

    m_x86Duration = computeDuration(m_time);

    if (!(*s_shouldExit))
    {
        // Wait for bridge to copy messages
        m_gpuEvent.wait();

        m_x64Duration = computeDuration(m_time);
        m_time = std::chrono::high_resolution_clock::now();

        m_gpuEvent.reset();

        memcpy(m_memoryMap, m_messages, m_offset);

        // Let bridge know we copied messages
        m_cpuEvent.set();
    }

    m_lastCommittedSize = m_offset;
    m_offset = sizeof(uint32_t);
}

void MessageSender::notifyShouldExit() const
{
    m_cpuEvent.reset();
    m_gpuEvent.set();
}

double MessageSender::getX86Duration() const
{
    return m_x86Duration;
}

double MessageSender::getX64Duration() const
{
    return m_x64Duration;
}

uint32_t MessageSender::getLastCommittedSize() const
{
    return m_lastCommittedSize;
}

