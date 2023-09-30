#include "CommandList.h"

#include "CommandQueue.h"

void CommandList::init(ID3D12Device* device, const CommandQueue& commandQueue)
{
    HRESULT hr = device->CreateCommandAllocator(commandQueue.getType(), IID_PPV_ARGS(m_commandAllocator.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_commandAllocator != nullptr);

    hr = device->CreateCommandList(0, commandQueue.getType(), m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_commandList != nullptr);
}

ID3D12GraphicsCommandList* CommandList::getUnderlyingCommandList() const
{
    return m_commandList.Get();
}

bool CommandList::isOpen() const
{
    return m_isOpen;
}

void CommandList::open()
{
    if (!m_isOpen)
    {
        HRESULT hr = m_commandAllocator->Reset();
        assert(SUCCEEDED(hr));

        hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
        assert(SUCCEEDED(hr));

        m_isOpen = true;
    }
}

void CommandList::close()
{
    if (m_isOpen)
    {
        for (auto& [resource, states] : m_resourceStates)
            states.stateAfter = D3D12_RESOURCE_STATE_COMMON;

        dispatchResourceBarriers();
        m_resourceStates.clear();

        const HRESULT hr = m_commandList->Close();
        assert(SUCCEEDED(hr));

        m_isOpen = false;
    }
}

void CommandList::setResourceState(ID3D12Resource* resource, D3D12_RESOURCE_STATES resourceState)
{
    m_resourceStates[resource].stateAfter = resourceState;
}

void CommandList::dispatchResourceBarriers()
{
    m_resourceBarriers.clear();

    for (auto& [resource, states] : m_resourceStates)
    {
        if (states.stateBefore != states.stateAfter)
        {
            m_resourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, states.stateBefore, states.stateAfter));
            states.stateBefore = states.stateAfter;
        }
    }

    if (!m_resourceBarriers.empty())
        m_commandList->ResourceBarrier(static_cast<UINT>(m_resourceBarriers.size()), m_resourceBarriers.data());
}
