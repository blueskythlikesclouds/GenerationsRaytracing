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
        const HRESULT hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
        assert(SUCCEEDED(hr));

        m_isOpen = true;
    }
}

void CommandList::close()
{
    if (m_isOpen)
    {
        const HRESULT hr = m_commandList->Close();
        assert(SUCCEEDED(hr));

        m_isOpen = false;
    }
}
