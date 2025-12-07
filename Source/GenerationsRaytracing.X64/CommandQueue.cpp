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

    hr = device->CreateFence(1, 
        D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_fence != nullptr);

#ifdef _DEBUG
    const wchar_t* name = nullptr;

    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        name = L"Graphics";
        break;

    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        name = L"Compute";
        break;

    case D3D12_COMMAND_LIST_TYPE_COPY:
        name = L"Copy";
        break;
    }

    wchar_t queueName[0x100];
    wcscpy_s(queueName, name);
    wcscat_s(queueName, L" Queue");
    m_queue->SetName(queueName);

    wchar_t fenceName[0x100];
    wcscpy_s(fenceName, name);
    wcscat_s(fenceName, L" Fence");
    m_fence->SetName(fenceName);
#endif
}

D3D12_COMMAND_LIST_TYPE CommandQueue::getType() const
{
    return m_type;
}

ID3D12CommandQueue* CommandQueue::getUnderlyingQueue() const
{
    return m_queue.Get();
}

ID3D12Fence* CommandQueue::getFence() const
{
    return m_fence.Get();
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

void CommandQueue::wait(uint64_t fenceValue, const CommandQueue& commandQueue) const
{
    const HRESULT hr = m_queue->Wait(commandQueue.m_fence.Get(), fenceValue);
    assert(SUCCEEDED(hr));
}
