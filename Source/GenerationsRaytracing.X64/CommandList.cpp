#include "CommandList.h"

#include "CommandQueue.h"

void CommandList::init(ID3D12Device* device, const CommandQueue& commandQueue)
{
    HRESULT hr = device->CreateCommandAllocator(commandQueue.getType(), IID_PPV_ARGS(m_commandAllocator.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_commandAllocator != nullptr);

    hr = device->CreateCommandList(0, commandQueue.getType(), m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.GetAddressOf()));
    assert(SUCCEEDED(hr) && m_commandList != nullptr);
}

ID3D12GraphicsCommandList4* CommandList::getUnderlyingCommandList() const
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
            states.stateAfter = states.stateInitial;

        commitBarriers();
        m_resourceStates.clear();

        const HRESULT hr = m_commandList->Close();
        assert(SUCCEEDED(hr));

        m_isOpen = false;
    }
}

void CommandList::transitionBarrier(ID3D12Resource* resource, 
    D3D12_RESOURCE_STATES stateInitial, D3D12_RESOURCE_STATES stateAfter)
{
    const auto result = m_resourceStates.find(resource);

    if (result != m_resourceStates.end())
    {
        assert(result->second.stateInitial == stateInitial);
        result->second.stateAfter = stateAfter;
    }
    else
    {
        m_resourceStates.emplace(resource, ResourceStates { stateInitial, stateInitial, stateAfter });
    }
}

void CommandList::transitionBarriers(std::initializer_list<ID3D12Resource*> resources,
    D3D12_RESOURCE_STATES stateInitial, D3D12_RESOURCE_STATES stateAfter)
{
    for (const auto resource : resources)
        transitionBarrier(resource, stateInitial, stateAfter);
}

void CommandList::uavBarriers(std::initializer_list<ID3D12Resource*> resources)
{
    for (const auto resource : resources)
        uavBarrier(resource);
}

void CommandList::uavBarrier(ID3D12Resource* resource)
{
    if (!m_uavResources.contains(resource))
    {
        m_resourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource));
        m_uavResources.emplace(resource);
    }
}

void CommandList::transitionAndUavBarrier(ID3D12Resource* resource, 
    D3D12_RESOURCE_STATES stateInitial, D3D12_RESOURCE_STATES stateAfter)
{
    uavBarrier(resource);
    transitionBarrier(resource, stateInitial, stateAfter);
}

void CommandList::transitionAndUavBarriers(std::initializer_list<ID3D12Resource*> resources,
    D3D12_RESOURCE_STATES stateInitial, D3D12_RESOURCE_STATES stateAfter)
{
    for (const auto resource : resources)
        transitionAndUavBarrier(resource, stateInitial, stateAfter);
}

void CommandList::commitBarriers()
{
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

    m_resourceBarriers.clear();
    m_uavResources.clear();
}
