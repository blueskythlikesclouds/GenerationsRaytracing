#include "MessageSender.h"

#include "LockGuard.h"
#include "Message.h"

bool MessageSender::canMakeMessage(uint32_t byteSize, uint32_t alignment)
{
    return ((sizeof(MessageQueue) + alignment - 1) & ~(alignment - 1)) + byteSize <= MemoryMappedFile::s_size;
}

MessageSender::MessageSender()
{
    m_memoryMap = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageSender::~MessageSender()
{
    m_memoryMappedFile.unmap(m_memoryMap);
}

struct MessageHolder
{
    uint32_t alignment = 0;
    std::vector<uint8_t> data;
};

struct MessageHolderStack : std::vector<MessageHolder>
{
    uint32_t peekIndex = 0;
};

static MessageHolderStack& getMessageHolderStack()
{
    thread_local MessageHolderStack stack;
    return stack;
}

void* MessageSender::makeMessage(uint32_t byteSize, uint32_t alignment)
{
    auto& stack = getMessageHolderStack();
    
    ++stack.peekIndex;
    if (stack.size() <= stack.peekIndex)
        stack.resize(stack.peekIndex + 1);

    auto& holder = stack[stack.peekIndex];
    holder.alignment = alignment;
    holder.data.resize(byteSize);

    return &holder.data[0];
}

void MessageSender::endMessage()
{
    auto& stack = getMessageHolderStack();
    auto& holder = stack[stack.peekIndex];

    LockGuard lock(m_mutex);

    uint32_t alignedOffset = m_offset;
    if ((alignedOffset & (holder.alignment - 1)) != 0)
    {
        alignedOffset += offsetof(MsgPadding, data);
        alignedOffset += holder.alignment - 1;
        alignedOffset &= ~(holder.alignment - 1);
    }
    if (alignedOffset + holder.data.size() > MemoryMappedFile::s_size)
    {
        sync();
        alignedOffset = std::max(sizeof(MessageQueue), holder.alignment);
    }

    if (m_offset != alignedOffset)
    {
        const auto padding = reinterpret_cast<MsgPadding*>(&m_memoryMap[m_offset]);
        padding->id = MsgPadding::s_id;
        padding->dataSize = alignedOffset - (m_offset + offsetof(MsgPadding, data));
    }

    memcpy(m_memoryMap + alignedOffset, &holder.data[0], holder.data.size());

    m_offset = alignedOffset + holder.data.size();
    MESSAGE_QUEUE->offset = m_offset;

    --stack.peekIndex;
}

void MessageSender::sync()
{
    LockGuard lock(m_mutex);
    m_offset = sizeof(MessageQueue);

    MESSAGE_QUEUE->sync = true;
    if (!(*s_shouldExit))
        m_x64Event.wait();

    MESSAGE_QUEUE->sync = false;
    MESSAGE_QUEUE->offset = m_offset;
    m_x86Event.set();
}

void MessageSender::notifyShouldExit()
{
    *s_shouldExit = true;

    while (!m_mutex.tryLock()) 
        m_x64Event.set();

    m_mutex.unlock();
}
