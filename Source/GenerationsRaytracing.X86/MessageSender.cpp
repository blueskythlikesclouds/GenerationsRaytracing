#include "MessageSender.h"

#include "LockGuard.h"

static constexpr size_t s_bufferSize = MemoryMappedFile::s_size / 2;

MessageSender::MessageSender() : m_pendingMessages(0)
{
    m_parallelBuffer = std::make_unique<uint8_t[]>(s_bufferSize);
    m_serialBuffer = std::make_unique<uint8_t[]>(s_bufferSize);
    m_tgtBufferData = static_cast<uint8_t*>(m_memoryMappedFile.map());
}

MessageSender::~MessageSender()
{
    m_memoryMappedFile.unmap(m_tgtBufferData);
}

void* MessageSender::makeParallelMessage(uint32_t byteSize, uint32_t alignment)
{
    assert(byteSize <= s_bufferSize); // some ridiculously large meshes/textures may hit this limit but like... don't?

    LockGuard lock(m_mutex);

    uint32_t offset = (m_parallelBufferSize + alignment - 1) & ~(alignment - 1);
    uint32_t newSize = offset + byteSize;

    if (newSize >= s_bufferSize) // no need for terminator since serial buffer is gonna have it
    {
        sendAllMessages();

        offset = 0;
        newSize = byteSize;
    }

    ++m_pendingMessages;

    if (m_parallelBufferSize != offset) // marker that says that the message is aligned
        m_parallelBuffer[m_parallelBufferSize] = static_cast<uint8_t>(0x80 | (offset - m_parallelBufferSize));

    m_parallelBufferSize = newSize;

    return m_parallelBuffer.get() + offset;
}

void MessageSender::endParallelMessage()
{
    --m_pendingMessages;
}

void* MessageSender::makeSerialMessage(uint32_t byteSize, uint32_t alignment)
{
    assert(m_serialCounter == 0);
    assert(byteSize < s_bufferSize);

#ifdef _DEBUG
    ++m_serialCounter;
#endif

    uint32_t offset = (m_serialBufferSize + alignment - 1) & ~(alignment - 1);
    uint32_t newSize = offset + byteSize;

    if (newSize > s_bufferSize) // need last byte as terminator
    {
        sendAllMessages();

        offset = 0;
        newSize = byteSize;
    }

    if (m_serialBufferSize != offset) // marker that says that the message is aligned
        m_serialBuffer[m_serialBufferSize] = static_cast<uint8_t>(0x80 | (offset - m_serialBufferSize));

    m_serialBufferSize = newSize;

    void* message = m_serialBuffer.get() + offset;
#ifdef _DEBUG
    --m_serialCounter;
#endif
    return message;
}

void MessageSender::sendAllMessages()
{
    LockGuard lock(m_mutex);

    while (m_pendingMessages != 0)
        ; // waste cycles, better than sleeping

    // align parallel buffer to 16 bytes
    const uint32_t alignedSize = (m_parallelBufferSize + 0xF) & ~0xF;

    if (m_parallelBufferSize != alignedSize)
    {
        m_parallelBuffer[m_parallelBufferSize] = static_cast<uint8_t>(0x80 | (alignedSize - m_parallelBufferSize));
        m_parallelBufferSize = alignedSize;
    }

    assert(m_serialBufferSize + m_parallelBufferSize < MemoryMappedFile::s_size);

    // wait for bridge to process messages
    m_gpuEvent.wait();
    m_gpuEvent.reset();

    memcpy(m_tgtBufferData, m_parallelBuffer.get(), m_parallelBufferSize);
    memcpy(m_tgtBufferData + m_parallelBufferSize, m_serialBuffer.get(), m_serialBufferSize);
    // MsgNullTerminator, end indicator
    *(m_tgtBufferData + m_parallelBufferSize + m_serialBufferSize) = '\0';

    // let bridge know we finished copying messages
    m_cpuEvent.set();

#if 0
    static int counter = 0;
    char path[256];
    sprintf(path, "C:/Work/MessageSender/%d.bin", ++counter);
    FILE* file = fopen(path, "wb");
    fwrite(m_parallelBuffer.get(), 1, m_parallelBufferSize, file);
    fwrite(m_serialBuffer.get(), 1, m_serialBufferSize, file);
    fclose(file);
#endif

    m_parallelBufferSize = 0;
    m_serialBufferSize = 0;
}

void MessageSender::setEvents() const
{
    m_cpuEvent.set();
    m_gpuEvent.set();
}

