#pragma once

class CommandList;

class CommandQueue
{
protected:
    D3D12_COMMAND_LIST_TYPE m_type{};
    ComPtr<ID3D12CommandQueue> m_queue;

    ComPtr<ID3D12Fence> m_fence;
    uint64_t m_fenceValue = 0;

    HANDLE m_event = nullptr;

    std::vector<ID3D12CommandList*> m_commandLists; // temp for executeCommandLists func

public:
    void init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type);

    D3D12_COMMAND_LIST_TYPE getType() const;
    ID3D12CommandQueue* getUnderlyingQueue() const;
    uint64_t getNextFenceValue();

    void executeCommandLists(size_t numCommandLists, CommandList* commandLists);
    void signal(uint64_t fenceValue) const;
    void wait(uint64_t fenceValue) const;
    void wait(uint64_t fenceValue, const CommandQueue& commandQueue) const;
};
