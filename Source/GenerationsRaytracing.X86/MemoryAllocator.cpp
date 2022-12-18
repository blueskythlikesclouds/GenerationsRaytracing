#include "MemoryAllocator.h"

// Copy pasted from GenerationsD3D9Ex:
// https://github.com/blueskythlikesclouds/DllMods/blob/master/Source/GenerationsD3D9Ex/MemoryHandler.cpp

constexpr const char* ALLOC_SIGNATURE = " GENSRT ";

HANDLE hHeap = GetProcessHeap();

HOOK(HANDLE*, __cdecl, GetHeapHandle, 0x660CF0, HANDLE& hHandle, size_t index)
{
    hHandle = hHeap;
    return &hHandle;
}

void* __cdecl alloc(const size_t byteSize)
{
    void* pMem = HeapAlloc(hHeap, 0, byteSize + 8);
    if (((uintptr_t)pMem % 16) == 0)
    {
        void* pMemRealloc = HeapReAlloc(hHeap, HEAP_REALLOC_IN_PLACE_ONLY, pMem, byteSize);
        return pMemRealloc ? pMemRealloc : pMem;
    }

    memcpy(pMem, ALLOC_SIGNATURE, 8);
    return &((uintptr_t*)pMem)[2];
}

void __cdecl dealloc(void* pMem)
{
    if (!pMem)
        return;

    uintptr_t* pSignature = &((uintptr_t*)pMem)[-2];
    if (memcmp(pSignature, ALLOC_SIGNATURE, 8) == 0)
    {
        memset(pSignature, 0, sizeof(*pSignature));
        HeapFree(hHeap, 0, pSignature);
    }
    else
    {
        HeapFree(hHeap, 0, pMem);
    }
}

void MemoryAllocator::init()
{
    // Don't init the memory allocator.
    WRITE_MEMORY(0x660C00, uint8_t, 0xC3);

    // Get a null handle.
    INSTALL_HOOK(GetHeapHandle);

    // Replace all functions.
    WRITE_JUMP(0x65FC60, alloc);
    WRITE_JUMP(0x65FC80, alloc);
    WRITE_JUMP(0x65FCA0, alloc);
    WRITE_JUMP(0x65FCC0, alloc);
    WRITE_JUMP(0x65FCE0, alloc);
    WRITE_JUMP(0x65FD00, alloc);
    WRITE_JUMP(0x65FD20, alloc);
    WRITE_JUMP(0x65FD40, alloc);
    WRITE_JUMP(0x65FD70, dealloc);
    WRITE_JUMP(0x65FD90, alloc);
    WRITE_JUMP(0x65FDB5, dealloc);
}
