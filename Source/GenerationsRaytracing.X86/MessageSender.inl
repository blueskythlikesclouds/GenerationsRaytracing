template <typename T>
T& MessageSender::makeMessage()
{
    void* message = makeMessage(sizeof(T), alignof(T));
    new (message) T();
    return *static_cast<T*>(message);
}

template <typename T>
T& MessageSender::makeMessage(uint32_t dataSize)
{
    void* message = makeMessage(offsetof(T, data) + dataSize, alignof(T));
    new (message) T();

    T* typedMessage = static_cast<T*>(message);
    typedMessage->dataSize = static_cast<decltype(T::dataSize)>(dataSize);
    return *typedMessage;
}