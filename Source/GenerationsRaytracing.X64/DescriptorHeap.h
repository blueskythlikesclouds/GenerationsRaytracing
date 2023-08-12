#pragma once

#include "SubAllocator.h"

class DescriptorHeap : public SubAllocator
{
protected:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    uint32_t m_incrementSize = 0;

public:
    void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index) const;
};
