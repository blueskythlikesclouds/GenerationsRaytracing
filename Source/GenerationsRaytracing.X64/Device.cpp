#include "Device.h"

#include "DxgiConverter.h"
#include "Message.h"

void Device::createBuffer(
    D3D12_HEAP_TYPE type,
    uint32_t dataSize,
    D3D12_RESOURCE_STATES initialState,
    ComPtr<D3D12MA::Allocation>& allocation) const
{
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = type;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

    assert(allocation == nullptr);

    const HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        allocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && allocation != nullptr);
}

void Device::writeBuffer(
    const uint8_t* memory, 
    uint32_t offset, 
    uint32_t dataSize, 
    ID3D12Resource* dstResource)
{
    auto& uploadBuffer = m_uploadBuffers.emplace_back();
    createBuffer(D3D12_HEAP_TYPE_UPLOAD, dataSize, D3D12_RESOURCE_STATE_GENERIC_READ, uploadBuffer);

    void* mappedData = nullptr;
    const HRESULT hr = uploadBuffer->GetResource()->Map(0, nullptr, &mappedData);

    assert(SUCCEEDED(hr) && mappedData != nullptr);

    memcpy(mappedData, memory, dataSize);

    uploadBuffer->GetResource()->Unmap(0, nullptr);

    m_copyCommandList.open();
    m_copyCommandList.getUnderlyingCommandList()->CopyBufferRegion(
        dstResource,
        offset,
        uploadBuffer->GetResource(),
        0,
        dataSize);
}

void Device::procMsgCreateSwapChain()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateSwapChain>();

    m_swapChain.procMsgCreateSwapChain(*this, message);
    m_swapChainTextureId = message.textureId;

    if (m_textures.size() <= m_swapChainTextureId)
        m_textures.resize(m_swapChainTextureId + 1);

    m_textures[m_swapChainTextureId] = m_swapChain.getTexture();
}

void Device::procMsgSetRenderTarget()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderTarget>();

    assert(message.renderTargetIndex == 0);

    if (message.textureId != NULL)
    {
        auto& texture = m_textures[message.textureId];
        if (texture.rtvIndex == NULL)
        {
            texture.rtvIndex = m_rtvDescriptorHeap.allocate();

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = DxgiConverter::makeColor(texture.format);
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            rtvDesc.Texture2D.PlaneSlice = 0;

            m_device->CreateRenderTargetView(
                texture.allocation->GetResource(),
                &rtvDesc,
                m_rtvDescriptorHeap.getCpuHandle(texture.rtvIndex));
        }
        m_renderTargetView = m_rtvDescriptorHeap.getCpuHandle(texture.rtvIndex);
    }
    else
    {
        m_renderTargetView.ptr = NULL;
    }
}

void Device::procMsgCreateVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexDeclaration>();

    if (m_vertexDeclarations.size() <= message.vertexDeclarationId)
        m_vertexDeclarations.resize(message.vertexDeclarationId + 1);
}

void Device::procMsgCreatePixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreatePixelShader>();

    if (m_pixelShaders.size() <= message.pixelShaderId)
        m_pixelShaders.resize(message.pixelShaderId + 1);

    auto& pixelShader = m_pixelShaders.emplace_back();

    pixelShader.byteCode = std::make_unique<uint8_t[]>(message.dataSize);
    pixelShader.byteSize = message.dataSize;

    memcpy(pixelShader.byteCode.get(), message.data, message.dataSize);
}

void Device::procMsgCreateVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexShader>();

    if (m_vertexShaders.size() <= message.vertexShaderId)
        m_vertexShaders.resize(message.vertexShaderId + 1);

    auto& vertexShader = m_vertexShaders.emplace_back();

    vertexShader.byteCode = std::make_unique<uint8_t[]>(message.dataSize);
    vertexShader.byteSize = message.dataSize;

    memcpy(vertexShader.byteCode.get(), message.data, message.dataSize);
}

void Device::procMsgSetRenderState()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetRenderState>();
}

void Device::procMsgCreateTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateTexture>();

    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = 0;
    resourceDesc.Width = static_cast<UINT64>(message.width);
    resourceDesc.Height = static_cast<UINT>(message.height);
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = static_cast<UINT16>(message.levels);
    resourceDesc.Format = DxgiConverter::convert(static_cast<D3DFORMAT>(message.format));

    if ((message.usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) != 0)
    {
        allocDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
        resourceDesc.Format = DxgiConverter::makeTypeless(resourceDesc.Format);
    }

    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if ((message.usage & D3DUSAGE_RENDERTARGET) != 0)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    if ((message.usage & D3DUSAGE_DEPTHSTENCIL) != 0)
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    if (m_textures.size() <= message.textureId)
        m_textures.resize(message.textureId + 1);

    auto& texture = m_textures[message.textureId];

    const HRESULT hr = m_allocator->CreateResource(
        &allocDesc,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        texture.allocation.GetAddressOf(),
        IID_ID3D12Resource,
        nullptr);

    assert(SUCCEEDED(hr) && texture.allocation != nullptr);

    texture.format = DxgiConverter::convert(static_cast<D3DFORMAT>(message.format));
}

void Device::procMsgSetTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetTexture>();

    if (message.textureId != NULL)
    {
        auto& texture = m_textures[message.textureId];

        if (texture.srvIndex == NULL)
        {
            texture.srvIndex = m_descriptorHeap.allocate();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = DxgiConverter::makeColor(texture.format);
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            m_device->CreateShaderResourceView(
                texture.allocation->GetResource(),
                &srvDesc,
                m_descriptorHeap.getCpuHandle(texture.srvIndex));
        }
        m_globalsPS.textureIndices[message.stage] = texture.srvIndex;
    }
    else
    {
        m_globalsPS.textureIndices[message.stage] = NULL;
    }
}

void Device::procMsgSetDepthStencilSurface()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetDepthStencilSurface>();

    if (message.depthStencilSurfaceId != NULL)
    {
        auto& texture = m_textures[message.depthStencilSurfaceId];

        if (texture.dsvIndex == NULL)
        {
            texture.dsvIndex = m_dsvDescriptorHeap.allocate();

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = DxgiConverter::makeDepth(texture.format);
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;

            m_device->CreateDepthStencilView(
                texture.allocation->GetResource(),
                &dsvDesc,
                m_dsvDescriptorHeap.getCpuHandle(texture.dsvIndex));
        }
        m_depthStencilView = m_dsvDescriptorHeap.getCpuHandle(texture.dsvIndex);
    }
    else
    {
        m_depthStencilView.ptr = NULL;
    }
}

void Device::procMsgClear()
{
    const auto& message = m_messageReceiver.getMessage<MsgClear>();
}

void Device::procMsgSetVertexShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShader>();

    if (message.vertexShaderId != NULL)
    {
        const auto& vertexShader = m_vertexShaders[message.vertexShaderId];

        m_pipelineDesc.VS.pShaderBytecode = vertexShader.byteCode.get();
        m_pipelineDesc.VS.BytecodeLength = static_cast<SIZE_T>(vertexShader.byteSize);
    }
    else
    {
        m_pipelineDesc.VS.pShaderBytecode = nullptr;
        m_pipelineDesc.VS.BytecodeLength = 0;
    }
}

void Device::procMsgSetPixelShader()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShader>();

    if (message.pixelShaderId != NULL)
    {
        const auto& pixelShader = m_pixelShaders[message.pixelShaderId];

        m_pipelineDesc.PS.pShaderBytecode = pixelShader.byteCode.get();
        m_pipelineDesc.PS.BytecodeLength = static_cast<SIZE_T>(pixelShader.byteSize);
    }
    else
    {
        m_pipelineDesc.PS.pShaderBytecode = nullptr;
        m_pipelineDesc.PS.BytecodeLength = 0;
    }
}

void Device::procMsgSetPixelShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetPixelShaderConstantF>();

    memcpy(m_globalsPS.floatConstants[message.startRegister], message.data, message.dataSize);
}

void Device::procMsgSetVertexShaderConstantF()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexShaderConstantF>();

    memcpy(m_globalsVS.floatConstants[message.startRegister], message.data, message.dataSize);
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

    m_viewport.TopLeftX = static_cast<float>(message.x);
    m_viewport.TopLeftY = static_cast<float>(message.y);
    m_viewport.Width = static_cast<float>(message.width);
    m_viewport.Height = static_cast<float>(message.height);
    m_viewport.MinDepth = message.minZ;
    m_viewport.MinDepth = message.maxZ;
}

void Device::procMsgSetScissorRect()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetScissorRect>();

    m_scissorRect.left = static_cast<LONG>(message.left);
    m_scissorRect.top = static_cast<LONG>(message.top);
    m_scissorRect.right = static_cast<LONG>(message.right);
    m_scissorRect.bottom = static_cast<LONG>(message.bottom);
}

void Device::procMsgSetVertexDeclaration()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetVertexDeclaration>();

    if (message.vertexDeclarationId != NULL)
    {
        const auto& vertexDeclaration = m_vertexDeclarations[message.vertexDeclarationId];

        m_pipelineDesc.InputLayout.pInputElementDescs = vertexDeclaration.inputElements.get();
        m_pipelineDesc.InputLayout.NumElements = static_cast<UINT>(vertexDeclaration.inputElementsSize);
    }
    else
    {
        m_pipelineDesc.InputLayout.pInputElementDescs = nullptr;
        m_pipelineDesc.InputLayout.NumElements = 0;
    }
}

void Device::procMsgDrawPrimitiveUP()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawPrimitiveUP>();

    flushState();
}

void Device::procMsgSetStreamSource()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetStreamSource>();

    auto& vertexBufferView = m_vertexBufferViews[message.streamNumber];

    if (message.streamDataId != NULL)
    {
        const auto& vertexBuffer = m_vertexBuffers[message.streamDataId];

        vertexBufferView.BufferLocation = vertexBuffer->GetResource()->GetGPUVirtualAddress() + message.offsetInBytes;
        vertexBufferView.SizeInBytes = static_cast<UINT>(vertexBuffer->GetSize() - message.offsetInBytes);
        vertexBufferView.StrideInBytes = static_cast<UINT>(message.stride);
    }
    else
    {
        vertexBufferView.BufferLocation = NULL;
        vertexBufferView.SizeInBytes = 0;
        vertexBufferView.StrideInBytes = 0;
    }
}

void Device::procMsgSetIndices()
{
    const auto& message = m_messageReceiver.getMessage<MsgSetIndices>();

    if (message.indexDataId != NULL)
    {
        const auto& indexBuffer = m_indexBuffers[message.indexDataId];

        m_indexBufferView.BufferLocation = indexBuffer->GetResource()->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = static_cast<UINT>(indexBuffer->GetSize());
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT; // Safe bet for now.
    }
    else
    {
        m_indexBufferView.BufferLocation = NULL;
        m_indexBufferView.SizeInBytes = 0;
        m_indexBufferView.Format = DXGI_FORMAT_UNKNOWN;
    }
}

void Device::procMsgPresent()
{
    const auto& message = m_messageReceiver.getMessage<MsgPresent>();
    m_shouldPresent = true;
}

void Device::procMsgCreateVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateVertexBuffer>();

    if (m_vertexBuffers.size() <= message.vertexBufferId)
        m_vertexBuffers.resize(message.vertexBufferId + 1);

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        message.length, 
        D3D12_RESOURCE_STATE_COMMON, 
        m_vertexBuffers[message.vertexBufferId]);
}

void Device::procMsgWriteVertexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteVertexBuffer>();

    writeBuffer(
        message.data,
        message.offset, 
        message.dataSize,
        m_vertexBuffers[message.vertexBufferId]->GetResource());
}

void Device::procMsgCreateIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgCreateIndexBuffer>();

    if (m_indexBuffers.size() <= message.indexBufferId)
        m_indexBuffers.resize(message.indexBufferId + 1);

    createBuffer(
        D3D12_HEAP_TYPE_DEFAULT,
        message.length,
        D3D12_RESOURCE_STATE_COMMON,
        m_indexBuffers[message.indexBufferId]);
}

void Device::procMsgWriteIndexBuffer()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteIndexBuffer>();

    writeBuffer(
        message.data,
        message.offset,
        message.dataSize,
        m_indexBuffers[message.indexBufferId]->GetResource());
}

void Device::procMsgWriteTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgWriteTexture>();
}

void Device::procMsgMakeTexture()
{
    const auto& message = m_messageReceiver.getMessage<MsgMakeTexture>();

    if (m_textures.size() <= message.textureId)
        m_textures.resize(message.textureId + 1);

    auto& texture = m_textures[message.textureId];

    std::vector<D3D12_SUBRESOURCE_DATA> subResources;
    bool isCubeMap = false;

    const HRESULT hr = DirectX::LoadDDSTextureFromMemory(
        m_device.Get(),
        m_allocator.Get(),
        message.data,
        message.dataSize,
        nullptr,
        texture.allocation.GetAddressOf(),
        subResources,
        0,
        nullptr,
        &isCubeMap);

    assert(SUCCEEDED(hr) && texture.allocation != nullptr);

    auto& uploadBuffer = m_uploadBuffers.emplace_back();

    createBuffer(
        D3D12_HEAP_TYPE_UPLOAD,
        static_cast<uint32_t>(GetRequiredIntermediateSize(texture.allocation->GetResource(), 0, static_cast<UINT>(subResources.size()))),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        uploadBuffer);

    m_copyCommandList.open();

    UpdateSubresources(
        m_copyCommandList.getUnderlyingCommandList(),
        texture.allocation->GetResource(),
        uploadBuffer->GetResource(),
        0,
        0,
        static_cast<UINT>(subResources.size()),
        subResources.data());

    const auto resourceDesc = texture.allocation->GetResource()->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format = resourceDesc.Format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (resourceDesc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
        assert(!isCubeMap);

        srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE1DARRAY : D3D12_SRV_DIMENSION_TEXTURE1D;

        if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE1DARRAY)
        {
            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.ArraySize = resourceDesc.DepthOrArraySize;
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
        }
        else
        {
            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
        }

        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
        if (isCubeMap)
        {
            srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 6 ? D3D12_SRV_DIMENSION_TEXTURECUBEARRAY : D3D12_SRV_DIMENSION_TEXTURECUBE;

            if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
            {
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = resourceDesc.MipLevels;
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = resourceDesc.DepthOrArraySize / 6;
                srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            }
            else
            {
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
        }
        else
        {
            srvDesc.ViewDimension = resourceDesc.DepthOrArraySize > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DARRAY : D3D12_SRV_DIMENSION_TEXTURE2D;

            if (srvDesc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY)
            {
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = resourceDesc.MipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = resourceDesc.DepthOrArraySize;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
            }
            else
            {
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
            }
        }

        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
        assert(!isCubeMap);

        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = resourceDesc.MipLevels;
        srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        break;

    default: 
        assert(false);
        break;
    }

    texture.srvIndex = m_descriptorHeap.allocate();

    m_device->CreateShaderResourceView(
        texture.allocation->GetResource(),
        &srvDesc,
        m_descriptorHeap.getCpuHandle(texture.srvIndex));
}

void Device::procMsgDrawIndexedPrimitive()
{
    const auto& message = m_messageReceiver.getMessage<MsgDrawIndexedPrimitive>();

    flushState();

    //m_graphicsCommandList.getUnderlyingCommandList()
    //    ->DrawIndexedInstanced(
    //        message.indexCount,
    //        1,
    //        message.startIndex,
    //        message.baseVertexIndex,
    //        0);
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

    m_graphicsCommandList.init(m_device.Get(), m_graphicsQueue);
    m_copyCommandList.init(m_device.Get(), m_copyQueue);

    m_descriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_samplerDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_rtvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_dsvDescriptorHeap.init(m_device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void Device::flushState()
{
    m_graphicsCommandList.getUnderlyingCommandList()
        ->OMSetRenderTargets(
            1, 
            m_renderTargetView.ptr != NULL ? &m_renderTargetView : nullptr, 
            FALSE, 
            m_depthStencilView.ptr != NULL ? &m_depthStencilView : nullptr);

    const XXH64_hash_t pipelineHash = XXH3_64bits(&m_pipelineDesc, sizeof(m_pipelineDesc));
    auto& pipeline = m_pipelines[pipelineHash];
    if (!pipeline)
    {
        const HRESULT hr = m_device->CreateGraphicsPipelineState(&m_pipelineDesc, IID_PPV_ARGS(pipeline.GetAddressOf()));
        assert(SUCCEEDED(hr));
    }
    m_graphicsCommandList.getUnderlyingCommandList()
        ->SetPipelineState(pipeline.Get());

    m_graphicsCommandList.getUnderlyingCommandList()
        ->SetComputeRootConstantBufferView(0, makeConstantBuffer(&m_globalsVS));

    m_graphicsCommandList.getUnderlyingCommandList()
        ->SetComputeRootConstantBufferView(1, makeConstantBuffer(&m_globalsPS));

    m_graphicsCommandList.getUnderlyingCommandList()
        ->RSSetViewports(1, &m_viewport);

    m_graphicsCommandList.getUnderlyingCommandList()
        ->RSSetScissorRects(1, &m_scissorRect);

    m_graphicsCommandList.getUnderlyingCommandList()
        ->IASetVertexBuffers(0, 16, m_vertexBufferViews);

    m_graphicsCommandList.getUnderlyingCommandList()
        ->IASetIndexBuffer(&m_indexBufferView);
}

void Device::resetState()
{
    m_uploadBuffers.clear();
    m_cbPool.resize(m_cbIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS Device::makeConstantBuffer(const void* memory)
{
    if (m_cbPool.size() <= m_cbIndex)
        m_cbPool.resize(m_cbIndex + 1);

    auto& buffer = m_cbPool[m_cbIndex];
    if (!buffer)
    {
        D3D12MA::ALLOCATION_DESC allocDesc{};
        allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(0x1000);

        const HRESULT hr = m_allocator->CreateResource(
            &allocDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            buffer.GetAddressOf(),
            IID_ID3D12Resource,
            nullptr);

        assert(SUCCEEDED(hr));
    }
    void* mappedData = nullptr;
    const HRESULT hr = buffer->GetResource()->Map(0, nullptr, &mappedData);
    assert(SUCCEEDED(hr) && mappedData != nullptr);

    memcpy(mappedData, memory, 0x1000);

    buffer->GetResource()->Unmap(0, nullptr);

    return buffer->GetResource()->GetGPUVirtualAddress();
}

void Device::processMessages()
{
    if (m_swapChainTextureId != 0)
        m_textures[m_swapChainTextureId] = m_swapChain.getTexture();

    m_messageReceiver.reset();

    m_graphicsCommandList.open();

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_descriptorHeap.getUnderlyingHeap(),
        m_samplerDescriptorHeap.getUnderlyingHeap()
    };
    m_graphicsCommandList.getUnderlyingCommandList()
        ->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // wait for Generations to copy the messages
    m_cpuEvent.wait();
    m_cpuEvent.reset();

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

    m_graphicsCommandList.close();

    // Execute copy command list if it was opened
    if (m_copyCommandList.isOpen())
    {
        m_copyCommandList.close();
        m_copyQueue.executeCommandList(m_copyCommandList);

        const uint64_t fenceValue = m_copyQueue.getNextFenceValue();
        m_copyQueue.signal(fenceValue);

        // Wait for copy command list to finish
        m_graphicsQueue.wait(fenceValue, m_copyQueue);
    }

    m_graphicsQueue.executeCommandList(m_graphicsCommandList);

    if (m_shouldPresent)
    {
        m_swapChain.present();
        m_shouldPresent = false;
    }

    // Wait for graphics command list to finish
    const uint64_t fenceValue = m_graphicsQueue.getNextFenceValue();
    m_graphicsQueue.signal(fenceValue);
    m_graphicsQueue.wait(fenceValue);

    // Cleanup
    resetState();
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
