#pragma once

class ArchiveDatabase
{
public:
    static hl::archive load(void* data, size_t dataSize);
    static hl::archive load(const std::string& filePath);
};
