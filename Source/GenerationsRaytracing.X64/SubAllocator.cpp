#include "SubAllocator.h"

SubAllocator::SubAllocator(
    uint32_t blockSize,
    D3D12_RESOURCE_FLAGS flags,
    uint32_t alignment,
    D3D12_RESOURCE_STATES initialState)
    : m_blockSize(blockSize), m_flags(flags), m_alignment(alignment), m_initialState(initialState)
{
}

SubAllocation SubAllocator::allocate(D3D12MA::Allocator* allocator, uint32_t byteSize)
{
    SubAllocation subAllocation{};
    subAllocation.byteSize = byteSize;

    // If requested resource is larger than half the block size, allocate it separately
    if (byteSize >= m_blockSize / 2)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize, m_flags);

        HRESULT hr = allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            m_initialState,
            nullptr,
            subAllocation.blockAllocation.GetAddressOf(),
            IID_ID3D12Resource,
            nullptr);

        assert(SUCCEEDED(hr) && subAllocation.blockAllocation != nullptr);
        return subAllocation;
    }

    D3D12MA::VIRTUAL_ALLOCATION_DESC virtualAllocDesc{};
    virtualAllocDesc.Size = byteSize;
    virtualAllocDesc.Alignment = m_alignment;

    UINT64 offset = 0;
    for (size_t i = 0; i < m_blocks.size(); i++)
    {
        const auto& block = m_blocks[i];
        HRESULT hr = block.virtualBlock->Allocate(&virtualAllocDesc, &subAllocation.virtualAllocation, &offset);
        if (SUCCEEDED(hr))
        {
            subAllocation.blockAllocation = block.blockAllocation;
            subAllocation.blockIndex = static_cast<uint32_t>(i);
            subAllocation.byteOffset = static_cast<uint32_t>(offset);

            return subAllocation;
        }
    }

    // Create new block if the resource couldn't fit to any of them
    subAllocation.blockIndex = static_cast<uint32_t>(m_blocks.size());

    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_blockSize, m_flags);

    auto& block = m_blocks.emplace_back();

    D3D12MA::VIRTUAL_BLOCK_DESC virtualBlockDesc{};
    virtualBlockDesc.Size = m_blockSize;

    HRESULT hr = D3D12MA::CreateVirtualBlock(&virtualBlockDesc, block.virtualBlock.GetAddressOf());
    assert(SUCCEEDED(hr) && block.virtualBlock != nullptr);

    hr = allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        m_initialState,
        nullptr,
        block.blockAllocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && block.blockAllocation != nullptr);

    hr = block.virtualBlock->Allocate(&virtualAllocDesc, &subAllocation.virtualAllocation, &offset);
    assert(SUCCEEDED(hr));

    subAllocation.blockAllocation = block.blockAllocation;
    subAllocation.byteOffset = static_cast<uint32_t>(offset);

    return subAllocation;
}

void SubAllocator::free(SubAllocation& allocation) const
{
    if (allocation.virtualAllocation.AllocHandle != NULL)
        m_blocks[allocation.blockIndex].virtualBlock->FreeAllocation(allocation.virtualAllocation);

    allocation = {};
}