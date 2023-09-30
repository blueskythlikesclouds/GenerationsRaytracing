#pragma once

class DescriptorHeap;
class CommandQueue;

class CommandList
{
protected:
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    bool m_isOpen = true;

    struct ResourceStates
    {
        D3D12_RESOURCE_STATES stateInitial = D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_STATES stateBefore = D3D12_RESOURCE_STATE_COMMON;
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_COMMON;
    };

    ankerl::unordered_dense::map<ID3D12Resource*, ResourceStates> m_resourceStates;
    std::vector<D3D12_RESOURCE_BARRIER> m_resourceBarriers;

public:
    void init(ID3D12Device* device, const CommandQueue& commandQueue);

    ID3D12GraphicsCommandList* getUnderlyingCommandList() const;

    bool isOpen() const;

    void open();
    void close();

    void setResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateInitial, D3D12_RESOURCE_STATES stateAfter);
    void dispatchResourceBarriers();
};
