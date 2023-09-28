#include "CommandQueue.h"

#include "CommandList.h"

void CommandQueue::init(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    m_type = type;

    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = type;

    HRESULT hr = device->CreateCommandQueue(
        &desc, IID_PPV_ARGS(m_queue.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_queue != nullptr);

    hr = device->CreateFence(0, 
        D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_fence != nullptr);

    m_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(m_event != nullptr);
}

D3D12_COMMAND_LIST_TYPE CommandQueue::getType() const
{
    return m_type;
}

ID3D12CommandQueue* CommandQueue::getUnderlyingQueue() const
{
    return m_queue.Get();
}

uint64_t CommandQueue::getNextFenceValue()
{
    return InterlockedIncrement(&m_fenceValue);
}

void CommandQueue::executeCommandList(const CommandList& commandList) const
{
    ID3D12CommandList* commandLists[] = { commandList.getUnderlyingCommandList() };
    m_queue->ExecuteCommandLists(1, commandLists);
}

void CommandQueue::signal(uint64_t fenceValue) const
{
    const HRESULT hr = m_queue->Signal(m_fence.Get(), fenceValue);
    assert(SUCCEEDED(hr));
}

void CommandQueue::wait(uint64_t fenceValue) const
{
    if (m_fence->GetCompletedValue() < fenceValue)
    {
        const HRESULT hr = m_fence->SetEventOnCompletion(fenceValue, m_event);
        assert(SUCCEEDED(hr));

        const DWORD result = WaitForSingleObject(m_event, INFINITE);
        assert(result == WAIT_OBJECT_0);
    }
}

void CommandQueue::wait(uint64_t fenceValue, const CommandQueue& commandQueue) const
{
    if (commandQueue.m_fence->GetCompletedValue() < fenceValue)
    {
        const HRESULT hr = m_queue->Wait(commandQueue.m_fence.Get(), fenceValue);
        assert(SUCCEEDED(hr));
    }
}
