#pragma once

#define DEFAULT_BLOCK_SIZE (64 * 1024 * 1024)

struct SubAllocation
{
    D3D12MA::VirtualAllocation virtualAllocation{};
    ComPtr<D3D12MA::Allocation> blockAllocation;
    uint32_t blockIndex = 0;
    uint32_t byteOffset = 0;
    uint32_t byteSize = 0;
};

class SubAllocator
{
protected:
    struct Block
    {
        ComPtr<D3D12MA::VirtualBlock> virtualBlock;
        ComPtr<D3D12MA::Allocation> blockAllocation;
    };

    std::vector<Block> m_blocks;
    uint32_t m_blockSize;
    D3D12_RESOURCE_FLAGS m_flags;
    uint32_t m_alignment = 0;
    D3D12_RESOURCE_STATES m_initialState;

public:
    SubAllocator(
        uint32_t blockSize,
        D3D12_RESOURCE_FLAGS flags,
        uint32_t alignment,
        D3D12_RESOURCE_STATES initialState);

    SubAllocation allocate(D3D12MA::Allocator* allocator, uint32_t byteSize);
    void free(SubAllocation& allocation) const;

    void freeBlocks(std::vector<ComPtr<D3D12MA::Allocation>>& blocksToFree);
};