#pragma once

namespace hl::text
{
    inline const nchar* strstr(const nchar* str1, const nchar* str2) noexcept
    {
#ifdef HL_IN_WIN32_UNICODE
        return wcsstr(str1, str2);
#else
        return strstr(str1, str2);
#endif
    }
}

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