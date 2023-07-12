#include <cassert>

inline MemoryMappedFile::MemoryMappedFile()
{
#ifdef _WIN64
    m_handle = OpenFileMapping(
        FILE_MAP_READ,
        FALSE,
        s_name);

    static_assert(sizeof(size_t) == 8);
#else
    m_handle = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        s_size,
        s_name);

    static_assert(sizeof(size_t) == 4);
#endif
    assert(m_handle != nullptr);
}

inline MemoryMappedFile::~MemoryMappedFile()
{
    assert(!m_mapped);
    const BOOL result = CloseHandle(m_handle);
    assert(result == TRUE);
}

inline void* MemoryMappedFile::map() const
{
    assert(!m_mapped);

    void* result = MapViewOfFile(
        m_handle,
#ifdef _WIN64
        FILE_MAP_READ,
#else
        FILE_MAP_WRITE,
#endif
        0,
        0,
        0);

    assert(result != nullptr);

#if _DEBUG
    m_mapped = true;
#endif
    return result;
}

inline void MemoryMappedFile::flush(void* buffer, size_t size) const
{
    const BOOL result = FlushViewOfFile(buffer, size);
    assert(result == TRUE);
}

inline void MemoryMappedFile::unmap(void* buffer) const
{
    assert(m_mapped);
    const BOOL result = UnmapViewOfFile(buffer);
    assert(result == TRUE);
#ifdef _DEBUG
    m_mapped = false;
#endif
}