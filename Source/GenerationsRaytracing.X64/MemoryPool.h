#pragma once

class MemoryPool
{
public:
    struct Allocation
    {

    };

protected:
    std::vector<ComPtr<D3D12MA::VirtualBlock>> m_virtualBlocks;
};