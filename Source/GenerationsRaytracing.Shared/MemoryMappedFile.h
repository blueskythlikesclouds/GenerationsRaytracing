#pragma once

#include <Windows.h>
#include <cassert>

#define MEMORY_MAPPED_FILE_SIZE (64 * 1024 * 1024)
#define MEMORY_MAPPED_FILE_NAME "GenerationsRaytracingMemoryMappedFile"

struct MemoryMappedFile
{
    HANDLE handle = nullptr;
#ifdef _DEBUG
    mutable bool mapped = false;
#endif

    MemoryMappedFile()
    {
#ifdef _WIN64
        handle = OpenFileMapping(
            FILE_MAP_READ,
            FALSE, 
            TEXT(MEMORY_MAPPED_FILE_NAME));

        static_assert(sizeof(size_t) == 8);
#else
        handle = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            MEMORY_MAPPED_FILE_SIZE,
            TEXT(MEMORY_MAPPED_FILE_NAME));

        static_assert(sizeof(size_t) == 4);
#endif
        assert(handle);
    }

    ~MemoryMappedFile()
    {
        assert(!mapped);
        const BOOL result = CloseHandle(handle);
        assert(result == TRUE);
    }

    void* map() const
    {
        assert(!mapped);

        void* result = MapViewOfFile(
            handle,
#ifdef _WIN64
            FILE_MAP_READ,
#else
            FILE_MAP_WRITE,
#endif
            0,
            0,
            0);

        assert(result);

#if _DEBUG
        mapped = true;
#endif
        return result;
    }

    void flush(void* buffer, size_t size) const
    {
        const BOOL result = FlushViewOfFile(buffer, size);
        assert(result == TRUE);
    }

    void unmap(void* buffer) const
    {
        assert(mapped);
        const BOOL result = UnmapViewOfFile(buffer);
        assert(result == TRUE);
#ifdef _DEBUG
        mapped = false;
#endif
    }

    struct Map
    {
        const MemoryMappedFile& memoryMappedFile;
        void* buffer = nullptr;

        Map(const MemoryMappedFile& memoryMappedFile)
            : memoryMappedFile(memoryMappedFile)
        {
            buffer = memoryMappedFile.map();
        }

        ~Map()
        {
            memoryMappedFile.unmap(buffer);
        }
    };
};