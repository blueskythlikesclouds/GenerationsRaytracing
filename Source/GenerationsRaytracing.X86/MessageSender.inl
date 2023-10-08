template <typename T>
T& MessageSender::makeMessage()
{
    T* message = static_cast<T*>(makeMessage(sizeof(T), alignof(T)));
    message->id = T::s_id;
    return *message;
}

template <typename T>
T& MessageSender::makeMessage(uint32_t dataSize)
{
    T* message = static_cast<T*>(makeMessage(offsetof(T, data) + dataSize, alignof(T)));
    message->id = T::s_id;
    message->dataSize = static_cast<decltype(T::dataSize)>(dataSize);
    return *message;
}