#include "CommandQueue.h"

void CommandQueue::init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = type;

    HRESULT hr = device->CreateCommandQueue(
        &desc, IID_PPV_ARGS(m_queue.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_queue != nullptr);

    hr = device->CreateFence(0, 
        D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_fence != nullptr);
}

ID3D12CommandQueue* CommandQueue::getUnderlyingQueue() const
{
    return m_queue.Get();
}
