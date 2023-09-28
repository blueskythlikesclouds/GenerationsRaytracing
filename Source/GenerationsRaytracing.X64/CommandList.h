#pragma once

class DescriptorHeap;
class CommandQueue;

class CommandList
{
protected:
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    bool m_isOpen = true;

public:
    void init(ID3D12Device* device, const CommandQueue& commandQueue);

    ID3D12GraphicsCommandList* getUnderlyingCommandList() const;

    bool isOpen() const;

    void open();
    void close();
};
