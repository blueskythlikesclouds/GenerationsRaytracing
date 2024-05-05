#include "SubAllocator.h"

D3D12_GPU_VIRTUAL_ADDRESS SubAllocation::getGpuVA() const
{
    return blockAllocation->GetResource()->GetGPUVirtualAddress() + byteOffset;
}

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
        if (block.blockAllocation != nullptr)
        {
            HRESULT hr = block.virtualBlock->Allocate(&virtualAllocDesc, &subAllocation.virtualAllocation, &offset);
            if (SUCCEEDED(hr))
            {
                subAllocation.blockAllocation = block.blockAllocation;
                subAllocation.blockIndex = static_cast<uint32_t>(i);
                subAllocation.byteOffset = static_cast<uint32_t>(offset);

                return subAllocation;
            }
        }
    }

    // Reuse or create new block if the resource couldn't fit to any of them

    bool foundBlock = false;
    for (uint32_t i = 0; i < m_blocks.size(); i++)
    {
        if (m_blocks[i].blockAllocation == nullptr)
        {
            subAllocation.blockIndex = i;
            foundBlock = true;
            break;
        }
    }

    if (!foundBlock)
        subAllocation.blockIndex = static_cast<uint32_t>(m_blocks.size());

    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_blockSize, m_flags);

    auto& block = !foundBlock ? 
        m_blocks.emplace_back() : m_blocks[subAllocation.blockIndex];

    if (!block.virtualBlock)
    {
        D3D12MA::VIRTUAL_BLOCK_DESC virtualBlockDesc{};
        virtualBlockDesc.Size = m_blockSize;

        const HRESULT hr = D3D12MA::CreateVirtualBlock(&virtualBlockDesc, block.virtualBlock.GetAddressOf());
        assert(SUCCEEDED(hr) && block.virtualBlock != nullptr);
    }

    HRESULT hr = allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        m_initialState,
        nullptr,
        block.blockAllocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && block.blockAllocation != nullptr);

#ifdef _DEBUG
    wchar_t name[0x100];
    swprintf_s(name, L"Sub Allocation Buffer %d", static_cast<uint32_t>(m_blocks.size()));
    block.blockAllocation->GetResource()->SetName(name);
#endif

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

void SubAllocator::freeBlocks(std::vector<ComPtr<D3D12MA::Allocation>>& blocksToFree)
{
    size_t count = 0;

    for (auto& block : m_blocks)
    {
        if (block.blockAllocation != nullptr && block.virtualBlock->IsEmpty())
            blocksToFree.emplace_back(std::move(block.blockAllocation));

        if (block.blockAllocation != nullptr)
            ++count;
    }

    while (!m_blocks.empty() && m_blocks.back().blockAllocation == nullptr)
        m_blocks.pop_back();
}