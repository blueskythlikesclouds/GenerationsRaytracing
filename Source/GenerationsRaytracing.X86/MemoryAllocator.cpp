#include "MemoryAllocator.h"

#define MEMORY_SIGNATURE " GensRT "
static_assert(sizeof(MEMORY_SIGNATURE) == (8 + 1));

HOOK(HANDLE*, __cdecl, GetHeapHandle, 0x660CF0, HANDLE& handle, size_t index)
{
    handle = GetProcessHeap();
    return &handle;
}

static void* __cdecl allocate(size_t byteSize)
{
    void* memory = HeapAlloc(GetProcessHeap(), 0, byteSize + 8);

    if ((reinterpret_cast<uintptr_t>(memory) & 0xF) == 0)
    {
        HeapReAlloc(GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY, memory, byteSize);
        return memory;
    }

    memcpy(memory, MEMORY_SIGNATURE, 8);
    return static_cast<uint8_t*>(memory) + 8;
}

static void __cdecl deallocate(void* memory)
{
    if (memory)
    {
        uint8_t* signature = static_cast<uint8_t*>(memory) - 8;

        if (memcmp(signature, MEMORY_SIGNATURE, 8) == 0)
        {
            memset(signature, 0, 8);
            HeapFree(GetProcessHeap(), 0, signature);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, memory);
        }
    }
}

void MemoryAllocator::init()
{
    // Don't init the memory allocator.
    WRITE_MEMORY(0x660C00, uint8_t, 0xC3);

    // Get the process handle.
    INSTALL_HOOK(GetHeapHandle);

    // Replace all functions.
    WRITE_JUMP(0x65FC60, allocate);
    WRITE_JUMP(0x65FC80, allocate);
    WRITE_JUMP(0x65FCA0, allocate);
    WRITE_JUMP(0x65FCC0, allocate);
    WRITE_JUMP(0x65FCE0, allocate);
    WRITE_JUMP(0x65FD00, allocate);
    WRITE_JUMP(0x65FD20, allocate);
    WRITE_JUMP(0x65FD40, allocate);
    WRITE_JUMP(0x65FD70, deallocate);
    WRITE_JUMP(0x65FD90, allocate);
    WRITE_JUMP(0x65FDB5, deallocate);
}
