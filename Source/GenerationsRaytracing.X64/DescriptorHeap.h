#pragma once

#include "FreeListAllocator.h"

class DescriptorHeap : public FreeListAllocator
{
protected:
    ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandle{};
    uint32_t m_incrementSize = 0;

public:
    void init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);

    ID3D12DescriptorHeap* getUnderlyingHeap() const;

    // Permanent allocation, cannot free!
    uint32_t allocateMany(uint32_t count);

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(uint32_t index) const;
    uint32_t getIncrementSize() const;
};
