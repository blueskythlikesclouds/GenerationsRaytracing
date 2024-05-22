#include "DescriptorHeap.h"

void DescriptorHeap::init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    const size_t capacity = (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
        D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1 : D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE) / 2;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};

    desc.Type = type;
    desc.NumDescriptors = static_cast<UINT>(capacity);

    desc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_heap.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_heap != nullptr);

    m_cpuDescriptorHandle = m_heap->GetCPUDescriptorHandleForHeapStart();

    if (desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        m_gpuDescriptorHandle = m_heap->GetGPUDescriptorHandleForHeapStart();

    m_incrementSize = device->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DescriptorHeap::getUnderlyingHeap() const
{
    return m_heap.Get();
}

uint32_t DescriptorHeap::allocateMany(uint32_t count)
{
    const uint32_t index = m_capacity;
    m_capacity += count;
    return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCpuHandle(uint32_t index) const
{
    return { m_cpuDescriptorHandle.ptr + m_incrementSize * static_cast<SIZE_T>(index) };
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGpuHandle(uint32_t index) const
{
    return { m_gpuDescriptorHandle.ptr + m_incrementSize * static_cast<SIZE_T>(index) };
}

uint32_t DescriptorHeap::getIncrementSize() const
{
    return m_incrementSize;
}

uint32_t DescriptorHeap::getIndex(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) const
{
    return static_cast<uint32_t>(cpuHandle.ptr - m_cpuDescriptorHandle.ptr) / m_incrementSize;
}

uint32_t DescriptorHeap::getIndex(D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) const
{
    return static_cast<uint32_t>(gpuHandle.ptr - m_gpuDescriptorHandle.ptr) / m_incrementSize;
}
