#pragma once

class CommandQueue
{
protected:
    D3D12_COMMAND_LIST_TYPE m_type{};
    ComPtr<ID3D12CommandQueue> m_queue;

    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValue = 0;

public:
    void init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

    ID3D12CommandQueue* getUnderlyingQueue() const;
};