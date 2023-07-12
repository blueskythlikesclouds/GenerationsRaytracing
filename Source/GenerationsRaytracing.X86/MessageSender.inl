template <typename T>
T& MessageSender::makeParallelMessage()
{
    void* message = makeParallelMessage(sizeof(T), alignof(T));
    new (message) T();
    return *static_cast<T*>(message);
}

template <typename T>
T& MessageSender::makeParallelMessage(uint32_t dataSize)
{
    void* message = makeParallelMessage(offsetof(T, data) + dataSize, alignof(T));
    new (message) T();

    T* typedMessage = static_cast<T*>(message);
    typedMessage->dataSize = static_cast<decltype(T::dataSize)>(dataSize);
    return *typedMessage;
}

template <typename T>
T& MessageSender::makeSerialMessage()
{
    void* message = makeSerialMessage(sizeof(T), alignof(T));
    new (message) T();
    return *static_cast<T*>(message);
}

template <typename T>
T& MessageSender::makeSerialMessage(uint32_t dataSize)
{
    void* message = makeSerialMessage(offsetof(T, data) + dataSize, alignof(T));
    new (message) T();

    T* typedMessage = static_cast<T*>(message);
    typedMessage->dataSize = static_cast<decltype(T::dataSize)>(dataSize);
    return *typedMessage;
}