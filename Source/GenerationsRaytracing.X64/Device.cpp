#include "Device.h"

#include "Message.h"

void Device::procMsgCreateSwapChain()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateSwapChain>();

    m_swapChain.procMsgCreateSwapChain(*this, message);
    m_swapChainTextureId = message.textureId;

    if (m_textures.size() <= m_swapChainTextureId)
        m_textures.resize(m_swapChainTextureId + 1);
}

void Device::procMsgSetRenderTarget()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderTarget>();
}

void Device::procMsgCreateVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexDeclaration>();
}

void Device::procMsgCreatePixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreatePixelShader>();
}

void Device::procMsgCreateVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexShader>();
}

void Device::procMsgSetRenderState()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderState>();
}

void Device::procMsgCreateTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateTexture>();
}

void Device::procMsgSetTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetTexture>();
}

void Device::procMsgSetDepthStencilSurface()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetDepthStencilSurface>();
}

void Device::procMsgClear()
{
    const auto& message = m_messageReceiver.getMessage<MsgClear>();
}

void Device::procMsgSetVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShader>();
}

void Device::procMsgSetPixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShader>();
}

void Device::procMsgSetPixelShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShaderConstantF>();
}

void Device::procMsgSetVertexShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShaderConstantF>();
}

void Device::procMsgSetVertexShaderConstantB()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShaderConstantB>();
}

void Device::procMsgSetSamplerState()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetSamplerState>();
}

void Device::procMsgSetViewport()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetViewport>();
}

void Device::procMsgSetScissorRect()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetScissorRect>();
}

void Device::procMsgSetVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexDeclaration>();
}

void Device::procMsgDrawPrimitiveUP()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawPrimitiveUP>();
}

void Device::procMsgSetStreamSource()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetStreamSource>();
}

void Device::procMsgSetIndices()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetIndices>();
}

void Device::procMsgPresent()
{
    const auto& message = m_messageReceiver.getMessage<MsgPresent>();
}

void Device::procMsgCreateVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexBuffer>();
}

void Device::procMsgWriteVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteVertexBuffer>();
}

void Device::procMsgCreateIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateIndexBuffer>();
}

void Device::procMsgWriteIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteIndexBuffer>();
}

void Device::procMsgWriteTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteTexture>();
}

void Device::procMsgMakeTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgMakeTexture>();
}

void Device::procMsgDrawIndexedPrimitive()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawIndexedPrimitive>();
}

void Device::procMsgSetStreamSourceFreq()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetStreamSourceFreq>();
}

Device::Device()
{
    HRESULT hr;

#ifdef _DEBUG
    ComPtr<ID3D12Debug> debugInterface;
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(debugInterface.GetAddressOf()));

    assert(SUCCEEDED(hr) && debugInterface != nullptr);

    debugInterface->EnableDebugLayer();
#endif

    hr = D3D12CreateDevice(m_swapChain.getAdapter(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_device.GetAddressOf()));

    assert(SUCCEEDED(hr) && m_device != nullptr);

    D3D12MA::ALLOCATOR_DESC desc{};

    desc.Flags = D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED;
    desc.pDevice = m_device.Get();
    desc.pAdapter = m_swapChain.getAdapter();

    hr = D3D12MA::CreateAllocator(&desc, m_allocator.GetAddressOf());

    assert(SUCCEEDED(hr) && m_allocator != nullptr);

    m_graphicsQueue.init(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_copyQueue.init(m_device.Get(), D3D12_COMMAND_LIST_TYPE_COPY);

    m_descriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_rtvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void Device::processMessages()
{
    if (m_swapChainTextureId != 0)
        m_textures[m_swapChainTextureId] = m_swapChain.getTexture();

    // wait for Generations to copy the messages
    m_cpuEvent.wait();
    m_cpuEvent.reset();

    m_messageReceiver.reset();

    bool stop = false;

    while (!stop && !m_swapChain.getWindow().m_shouldExit)
    {
        switch (m_messageReceiver.getId())
        {
        case MsgNullTerminator::s_id:
            stop = true;
            break;

        case MsgCreateSwapChain::s_id:
            procMsgCreateSwapChain();
            break;

        case MsgSetRenderTarget::s_id:
            procMsgSetRenderTarget();
            break;

        case MsgCreateVertexDeclaration::s_id:
            procMsgCreateVertexDeclaration();
            break;

        case MsgCreatePixelShader::s_id:
            procMsgCreatePixelShader();
            break;

        case MsgCreateVertexShader::s_id:
            procMsgCreateVertexShader();
            break;

        case MsgSetRenderState::s_id:
            procMsgSetRenderState();
            break;

        case MsgCreateTexture::s_id:
            procMsgCreateTexture();
            break;

        case MsgSetTexture::s_id:
            procMsgSetTexture();
            break;

        case MsgSetDepthStencilSurface::s_id:
            procMsgSetDepthStencilSurface();
            break;

        case MsgClear::s_id:
            procMsgClear();
            break;

        case MsgSetVertexShader::s_id:
            procMsgSetVertexShader();
            break;

        case MsgSetPixelShader::s_id:
            procMsgSetPixelShader();
            break;

        case MsgSetPixelShaderConstantF::s_id:
            procMsgSetPixelShaderConstantF();
            break;

        case MsgSetVertexShaderConstantF::s_id:
            procMsgSetVertexShaderConstantF();
            break;

        case MsgSetVertexShaderConstantB::s_id:
            procMsgSetVertexShaderConstantB();
            break;

        case MsgSetSamplerState::s_id:
            procMsgSetSamplerState();
            break;

        case MsgSetViewport::s_id:
            procMsgSetViewport();
            break;

        case MsgSetScissorRect::s_id:
            procMsgSetScissorRect();
            break;

        case MsgSetVertexDeclaration::s_id: 
            procMsgSetVertexDeclaration();
            break;

        case MsgDrawPrimitiveUP::s_id:
            procMsgDrawPrimitiveUP();
            break;

        case MsgSetStreamSource::s_id:
            procMsgSetStreamSource();
            break;

        case MsgSetIndices::s_id: 
            procMsgSetIndices();
            break;

        case MsgPresent::s_id: 
            procMsgPresent();
            break;

        case MsgCreateVertexBuffer::s_id:
            procMsgCreateVertexBuffer();
            break;

        case MsgWriteVertexBuffer::s_id: 
            procMsgWriteVertexBuffer();
            break;

        case MsgCreateIndexBuffer::s_id: 
            procMsgCreateIndexBuffer();
            break;

        case MsgWriteIndexBuffer::s_id: 
            procMsgWriteIndexBuffer();
            break;

        case MsgWriteTexture::s_id: 
            procMsgWriteTexture();
            break;

        case MsgMakeTexture::s_id: 
            procMsgMakeTexture();
            break;

        case MsgDrawIndexedPrimitive::s_id:
            procMsgDrawIndexedPrimitive();
            break;

        case MsgSetStreamSourceFreq::s_id:
            procMsgSetStreamSourceFreq();
            break;

        default:
            assert(!"Unknown message type");
            stop = true;
            break;
        }
    }

    // let Generations know we finished processing messages
    m_gpuEvent.set();
}

void Device::runLoop()
{
    while (!m_swapChain.getWindow().m_shouldExit)
    {
        m_swapChain.getWindow().processMessages();
        processMessages();
    }
}

void Device::setShouldExit()
{
    m_cpuEvent.set();
    m_gpuEvent.set();

    m_swapChain.getWindow().m_shouldExit = true;
}

ID3D12Device* Device::getUnderlyingDevice() const
{
    return m_device.Get();
}

CommandQueue& Device::getGraphicsQueue()
{
    return m_graphicsQueue;
}

CommandQueue& Device::getCopyQueue()
{
    return m_copyQueue;
}

DescriptorHeap& Device::getDescriptorHeap()
{
    return m_descriptorHeap;
}

DescriptorHeap& Device::getSamplerDescriptorHeap()
{
    return m_samplerDescriptorHeap;
}

DescriptorHeap& Device::getRtvDescriptorHeap()
{
    return m_rtvDescriptorHeap;
}

DescriptorHeap& Device::getDsvDescriptorHeap()
{
    return m_dsvDescriptorHeap;
}
