#pragma once

inline std::string getDirectoryPath(const std::string& path)
{
    const size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        return path.substr(0, index);

    return std::string();
}

inline std::string getFileName(const std::string& path)
{
    const size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        return path.substr(index + 1, path.size() - index - 1);

    return path;
}

inline std::string getFileNameWithoutExtension(const std::string& path)
{
    std::string fileName;

    size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        fileName = path.substr(index + 1, path.size() - index - 1);
    else
        fileName = path;

    index = fileName.find_last_of('.');
    if (index != std::string::npos)
        fileName = fileName.substr(0, index);

    return fileName;
}

template<size_t N = 2048>
inline std::array<hl::nchar, N> toNchar(const char* value)
{
    std::array<hl::nchar, N> array {};
    hl::text::utf8_to_native::conv(value, 0, array.data(), N);
    return array;
}

template<size_t N = 2048>
inline std::array<hl::nchar, N> toNchar(const std::string& value)
{
    return toNchar<N>(value.c_str());
}

template<size_t N = 2048>
inline std::array<char, N> fromNchar(const hl::nchar* value)
{
    std::array<char, N> array {};
    hl::text::native_to_utf8::conv(value, 0, array.data(), N);
    return array;
}

template<size_t N = 2048>
inline std::array<char, N> fromNchar(const hl::nstring& value)
{
    return fromNchar<N>(value.c_str());
}

