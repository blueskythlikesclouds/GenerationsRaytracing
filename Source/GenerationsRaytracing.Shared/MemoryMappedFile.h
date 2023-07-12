#pragma once

#include <Windows.h>

class MemoryMappedFile
{
protected:
    HANDLE m_handle = nullptr;
#ifdef _DEBUG
    mutable bool m_mapped = false;
#endif

public:
    static constexpr size_t s_size = 16 * 1024 * 1024;
    static constexpr TCHAR s_name[] = TEXT("GenerationsRaytracingMemoryMappedFile");

    MemoryMappedFile();
    ~MemoryMappedFile();

    void* map() const;
    void flush(void* buffer, size_t size) const;
    void unmap(void* buffer) const;
};

#include "MemoryMappedFile.inl"