#pragma once

#include "FreeListAllocator.h"

class DescriptorHeap : public FreeListAllocator
{
protected:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    uint32_t m_incrementSize = 0;

public:
    void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);

    ID3D12DescriptorHeap* getUnderlyingHeap() const;

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index) const;
};
