#pragma once

#include <string>

inline std::string getDirectoryPath(const std::string& path)
{
    const size_t index = path.find_last_of("\\/");
    if (index != std::string::npos)
        return path.substr(0, index);

    return ".";
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