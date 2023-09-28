#include "DescriptorHeap.h"

void DescriptorHeap::init(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    const size_t capacity = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
        D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1 : D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};

    desc.Type = type;
    desc.NumDescriptors = static_cast<UINT>(capacity);

    desc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    const HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_heap.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_heap != nullptr);

    m_incrementSize = device->GetDescriptorHandleIncrementSize(type);
}

ID3D12DescriptorHeap* DescriptorHeap::getUnderlyingHeap() const
{
    return m_heap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::getCpuHandle(uint32_t index) const
{
    auto handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += m_incrementSize * static_cast<SIZE_T>(index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::getGpuHandle(uint32_t index) const
{
    auto handle = m_heap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += m_incrementSize * static_cast<SIZE_T>(index);
    return handle;
}
