#pragma once

inline std::unique_ptr<uint8_t[]> readAllBytes(const std::string& filePath, size_t& fileSize)
{
    FILE* file = fopen(filePath.c_str(), "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    fileSize = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    auto bytes = std::make_unique<uint8_t[]>(fileSize);
    fread(bytes.get(), 1, fileSize, file);

    return bytes;
}