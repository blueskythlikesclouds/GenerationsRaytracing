#pragma once

struct MemoryAllocator
{
    static void init(ID3D12Device* device, IDXGIAdapter* adapter);
};
