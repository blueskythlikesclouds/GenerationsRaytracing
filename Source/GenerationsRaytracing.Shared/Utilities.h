#pragma once

#include <Windows.h>
#include <vector>

template<typename T>
inline size_t vectorByteSize(const std::vector<T>& vector)
{
    return vector.size() * sizeof(T);
}

template<typename T>
inline size_t vectorByteOffset(const std::vector<T>& vector, size_t index)
{
    return index * sizeof(T);
}

template<typename T>
inline size_t vectorByteStride(const std::vector<T>& vector)
{
    return sizeof(T);
}

inline void outputDebugString(const char* fmt, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsprintf(buffer, fmt, args);
    va_end(args);

    OutputDebugStringA(buffer);
}

inline uint32_t nextPowerOfTwo(uint32_t value)
{
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}