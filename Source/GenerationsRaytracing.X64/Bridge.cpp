#include "Bridge.h"

#include "Format.h"
#include "FVF.h"
#include "Hash.h"
#include "Message.h"

Bridge::Bridge(const std::string& directoryPath) : directoryPath(directoryPath)
{
    commandList = device.nvrhi->createCommandList(nvrhi::CommandListParameters().setEnableImmediateExecution(false));
    commandListForCopy = device.nvrhi->createCommandList(nvrhi::CommandListParameters().setEnableImmediateExecution(false));

    vsConstantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setDebugName("VS Constants")
        .setByteSize(sizeof(vsConstants.c))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    psConstantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setDebugName("PS Constants")
        .setByteSize(sizeof(psConstants.c))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    sharedConstantBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setDebugName("Shared Constants")
        .setByteSize(sizeof(sharedConstants))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1));

    vsBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Vertex)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)));

    vsBindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .setTrackLiveness(false)
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, sharedConstantBuffer)), vsBindingLayout);

    psBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)));

    psBindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
        .setTrackLiveness(false)
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, psConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, sharedConstantBuffer)), psBindingLayout);

    nullTexture = device.nvrhi->createTexture(nvrhi::TextureDesc()
        .setDebugName("Null Texture")
        .setWidth(1)
        .setHeight(1)
        .setFormat(nvrhi::Format::R8_UNORM)
        .setInitialState(nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true));

    textureBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(0))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(1))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(2))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(3))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(4))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(5))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(6))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(7))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(8))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(9))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(10))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(11))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(12))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(13))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(14))
        .addItem(nvrhi::BindingLayoutItem::Texture_SRV(15)));

    textureBindingSetDesc
        .setTrackLiveness(false)
        .addItem(nvrhi::BindingSetItem::Texture_SRV(0, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(1, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(2, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(3, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(4, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(5, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(6, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(7, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(8, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(9, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(10, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(11, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(12, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(13, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(14, nullTexture))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(15, nullTexture));

    samplerBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::Sampler(0))
        .addItem(nvrhi::BindingLayoutItem::Sampler(1))
        .addItem(nvrhi::BindingLayoutItem::Sampler(2))
        .addItem(nvrhi::BindingLayoutItem::Sampler(3))
        .addItem(nvrhi::BindingLayoutItem::Sampler(4))
        .addItem(nvrhi::BindingLayoutItem::Sampler(5))
        .addItem(nvrhi::BindingLayoutItem::Sampler(6))
        .addItem(nvrhi::BindingLayoutItem::Sampler(7))
        .addItem(nvrhi::BindingLayoutItem::Sampler(8))
        .addItem(nvrhi::BindingLayoutItem::Sampler(9))
        .addItem(nvrhi::BindingLayoutItem::Sampler(10))
        .addItem(nvrhi::BindingLayoutItem::Sampler(11))
        .addItem(nvrhi::BindingLayoutItem::Sampler(12))
        .addItem(nvrhi::BindingLayoutItem::Sampler(13))
        .addItem(nvrhi::BindingLayoutItem::Sampler(14))
        .addItem(nvrhi::BindingLayoutItem::Sampler(15)));

    samplerBindingSetDesc
        .setTrackLiveness(false)
        .addItem(nvrhi::BindingSetItem::Sampler(0, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(1, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(2, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(3, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(4, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(5, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(6, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(7, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(8, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(9, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(10, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(11, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(12, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(13, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(14, nullptr))
        .addItem(nvrhi::BindingSetItem::Sampler(15, nullptr));

    fvfShader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), FVF, sizeof(FVF));

    pipelineDesc.bindingLayouts.push_back(vsBindingLayout);
    pipelineDesc.bindingLayouts.push_back(psBindingLayout);
    pipelineDesc.bindingLayouts.push_back(textureBindingLayout);
    pipelineDesc.bindingLayouts.push_back(samplerBindingLayout);

    graphicsState.bindings.push_back(vsBindingSet);
    graphicsState.bindings.push_back(psBindingSet);
    graphicsState.bindings.emplace_back();
    graphicsState.bindings.emplace_back();
}

void Bridge::openCommandList()
{
    if (openedCommandList)
        return;

    commandList->open();
    openedCommandList = true;
}

void Bridge::openCommandListForCopy()
{
    if (openedCommandListForCopy)
        return;

    commandListForCopy->open();
    openedCommandListForCopy = true;
}

void Bridge::closeAndExecuteCommandLists()
{
    nvrhi::ICommandList* commandLists[] = { nullptr, nullptr };
    size_t count = 0;

    if (openedCommandListForCopy)
    {
        commandListForCopy->close();
        openedCommandListForCopy = false;

        commandLists[count++] = commandListForCopy.Get();
    }

    if (openedCommandList)
    {
        commandList->close();
        openedCommandList = false;

        commandLists[count++] = commandList.Get();
    }

    if (count > 0)
    {
        device.nvrhi->executeCommandLists(commandLists, count);
        device.nvrhi->waitForIdle();
        device.nvrhi->runGarbageCollection();

        dirtyFlags = DirtyFlags::All;
    }
}

void Bridge::processDirtyFlags()
{
    if ((dirtyFlags & DirtyFlags::VsConstants) != DirtyFlags::None)
        vsConstants.writeBuffer(commandList, vsConstantBuffer);

    if ((dirtyFlags & DirtyFlags::PsConstants) != DirtyFlags::None)
        psConstants.writeBuffer(commandList, psConstantBuffer);

    if ((dirtyFlags & DirtyFlags::SharedConstants) != DirtyFlags::None)
        commandList->writeBuffer(sharedConstantBuffer, &sharedConstants, sizeof(sharedConstants));

    XXH64_hash_t hash = 0;

    auto curTextureBindingSetDesc = this->textureBindingSetDesc;

    if (!framebufferDesc.colorAttachments.empty() && 
        ((dirtyFlags & DirtyFlags::Texture) != DirtyFlags::None ||
        (dirtyFlags & DirtyFlags::FramebufferAndPipeline) != DirtyFlags::None))
    {
        for (uint32_t i = 0; i < 16; i++)
        {
            if (curTextureBindingSetDesc.bindings[i].resourceHandle != framebufferDesc.colorAttachments[0].texture)
                continue;

            curTextureBindingSetDesc.bindings[i] = nvrhi::BindingSetItem::Texture_SRV(i, nullTexture);
            dirtyFlags |= DirtyFlags::Texture;
        }
    }

    if ((dirtyFlags & DirtyFlags::Texture) != DirtyFlags::None)
    {
        auto& textureBindingSet = textureBindingSets[hash = computeHash(curTextureBindingSetDesc.bindings.data(), curTextureBindingSetDesc.bindings.size(), 0)];
        if (!textureBindingSet)
            textureBindingSet = device.nvrhi->createBindingSet(curTextureBindingSetDesc, textureBindingLayout);

        assignAndUpdateDirtyFlags(graphicsState.bindings[2], textureBindingSet, DirtyFlags::GraphicsState);
    }

    if ((dirtyFlags & DirtyFlags::Sampler) != DirtyFlags::None)
    {
        for (uint32_t i = 0; i < 16; i++)
        {
            auto& sampler = samplers[hash = computeHash(samplerDescs[i], 0)];
            if (!sampler)
                sampler = device.nvrhi->createSampler(samplerDescs[i]);

            samplerBindingSetDesc.bindings[i] = nvrhi::BindingSetItem::Sampler(i, sampler);
        }

        auto& samplerBindingSet = samplerBindingSets[hash = computeHash(samplerBindingSetDesc.bindings.data(), samplerBindingSetDesc.bindings.size(), 0)];
        if (!samplerBindingSet)
            samplerBindingSet = device.nvrhi->createBindingSet(samplerBindingSetDesc, samplerBindingLayout);

        assignAndUpdateDirtyFlags(graphicsState.bindings[3], samplerBindingSet, DirtyFlags::GraphicsState);
    }

    if ((dirtyFlags & DirtyFlags::InputLayout) != DirtyFlags::None)
    {
        auto& attributes = vertexAttributeDescs[vertexDeclaration];
        if (!attributes.empty())
        {
            for (auto& attribute : attributes)
            {
                attribute.elementStride = vertexStrides[attribute.bufferIndex];
                attribute.isInstanced = attribute.bufferIndex > 0 && instancing;
            }

            hash = computeHash(attributes.data(), attributes.size(), 0);
            auto& inputLayout = inputLayouts[hash];
            if (!inputLayout)
                inputLayout = device.nvrhi->createInputLayout(attributes.data(), (uint32_t)attributes.size(), nullptr);

           assignAndUpdateDirtyFlags(pipelineDesc.inputLayout, inputLayout, DirtyFlags::FramebufferAndPipeline);
        }
        else
        {
            assignAndUpdateDirtyFlags(pipelineDesc.inputLayout, nullptr, DirtyFlags::FramebufferAndPipeline);
        }
    }

    auto curFramebufferDesc = this->framebufferDesc;
    auto curPipelineDesc = this->pipelineDesc;

    if (!curFramebufferDesc.colorAttachments.empty() && curFramebufferDesc.depthAttachment.texture)
    {
        auto& textureDesc = curFramebufferDesc.colorAttachments[0].texture->getDesc();

        if (textureDesc.width == 320 && textureDesc.height == 180)
            assignAndUpdateDirtyFlags(curFramebufferDesc.depthAttachment.texture, nullptr, DirtyFlags::FramebufferAndPipeline);
    }
    if (!curFramebufferDesc.depthAttachment.texture &&
        (curPipelineDesc.renderState.depthStencilState.depthTestEnable ||
            curPipelineDesc.renderState.depthStencilState.depthWriteEnable))
    {
        assignAndUpdateDirtyFlags(curPipelineDesc.renderState.depthStencilState.depthTestEnable, false, DirtyFlags::FramebufferAndPipeline);
        assignAndUpdateDirtyFlags(curPipelineDesc.renderState.depthStencilState.depthWriteEnable, false, DirtyFlags::FramebufferAndPipeline);
    }

    if ((dirtyFlags & DirtyFlags::FramebufferAndPipeline) != DirtyFlags::None)
    {
        hash = computeHash(curFramebufferDesc.colorAttachments.data(), curFramebufferDesc.colorAttachments.size(), 0);
        hash = computeHash(curFramebufferDesc.depthAttachment, hash);

        auto& framebuffer = framebuffers[hash];
        if (!framebuffer)
            framebuffer = device.nvrhi->createFramebuffer(curFramebufferDesc);

        if (fvf)
            curPipelineDesc.VS = fvfShader;

        hash = computeHash(curPipelineDesc, hash);
        auto& pipeline = pipelines[hash];
        if (!pipeline)
            pipeline = device.nvrhi->createGraphicsPipeline(curPipelineDesc, framebuffer);

        assignAndUpdateDirtyFlags(graphicsState.framebuffer, framebuffer, DirtyFlags::GraphicsState);
        assignAndUpdateDirtyFlags(graphicsState.pipeline, pipeline, DirtyFlags::GraphicsState);
    }

    if ((dirtyFlags & DirtyFlags::VertexBuffer) != DirtyFlags::None)
    {
        size_t vertexBufferCount;
        for (vertexBufferCount = 0; vertexBufferCount < graphicsState.vertexBuffers.size() &&
            graphicsState.vertexBuffers[vertexBufferCount].buffer; vertexBufferCount++)
            ;

        if (graphicsState.vertexBuffers.size() != vertexBufferCount)
            dirtyFlags |= DirtyFlags::GraphicsState;

        graphicsState.vertexBuffers.resize(vertexBufferCount);
    }

    if ((dirtyFlags & DirtyFlags::GraphicsState) != DirtyFlags::None)
        commandList->setGraphicsState(graphicsState);

    commandList->commitBarriers();

    dirtyFlags = DirtyFlags::None;
}

void Bridge::procMsgSetFVF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetFVF>();

    auto& attributes = vertexAttributeDescs[msg->vertexDeclaration];
    if (attributes.empty())
    {
        uint32_t offset = 0;

        if (msg->fvf & D3DFVF_XYZRHW)
        {
            attributes.emplace_back()
                .setName("POSITION")
                .setOffset(offset)
                .setFormat(nvrhi::Format::RGBA32_FLOAT);

            offset += 16;
        }

        if (msg->fvf & D3DFVF_DIFFUSE)
        {
            attributes.emplace_back()
                .setName("COLOR")
                .setOffset(offset)
                .setFormat(nvrhi::Format::BGRA8_UNORM);

            offset += 4;
        }

        if (msg->fvf & D3DFVF_TEX1)
        {
            attributes.emplace_back()
                .setName("TEXCOORD")
                .setOffset(offset)
                .setFormat(nvrhi::Format::RG32_FLOAT);

            offset += 8;
        }
    }

    assignAndUpdateDirtyFlags(vertexDeclaration, msg->vertexDeclaration, DirtyFlags::InputLayout);
    assignAndUpdateDirtyFlags(pipelineDesc.VS, fvfShader, DirtyFlags::FramebufferAndPipeline);

    if (!fvf)
    {
        fvf = true;
        dirtyFlags |= DirtyFlags::FramebufferAndPipeline;
    }
}

void Bridge::procMsgInitSwapChain()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgInitSwapChain>();

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = msg->width;
    swapChainDesc.Height = msg->height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = msg->bufferCount;
    swapChainDesc.Scaling = (DXGI_SCALING)msg->scaling;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ComPtr<IDXGISwapChain1> swapChain1;

    device.dxgiFactory->CreateSwapChainForHwnd(
        device.d3d12.graphicsCommandQueue.Get(),
        (HWND)(LONG_PTR)msg->handle,
        &swapChainDesc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf());

    assert(swapChain1);

    swapChain1.As(&swapChain);
    assert(swapChain);

    swapChainSurface = msg->surface;

    for (uint32_t i = 0; i < msg->bufferCount; i++)
    {
        ComPtr<ID3D12Resource> backBuffer;
        swapChain->GetBuffer(i, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        assert(backBuffer);

        const auto textureDesc = nvrhi::TextureDesc()
            .setDimension(nvrhi::TextureDimension::Texture2D)
            .setFormat(nvrhi::Format::RGBA8_UNORM)
            .setWidth(swapChainDesc.Width)
            .setHeight(swapChainDesc.Height)
            .setIsRenderTarget(true)
            .setInitialState(nvrhi::ResourceStates::Present)
            .setKeepInitialState(true);

        swapChainTextures.push_back(device.nvrhi->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, backBuffer.Get(), textureDesc));
    }
}

void Bridge::procMsgPresent()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgPresent>();
    shouldPresent = true;
}

void Bridge::procMsgCreateTexture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateTexture>();

    const auto format = Format::convert(msg->format);
    if (format == nvrhi::Format::UNKNOWN)
        return;

    const bool isRenderTargetOrDepthStencil = (msg->usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) != 0;

    resources[msg->texture] = device.nvrhi->createTexture(nvrhi::TextureDesc()
        .setWidth(msg->width)
        .setHeight(msg->height)
        .setMipLevels(msg->levels)
        .setIsRenderTarget(isRenderTargetOrDepthStencil)
        .setInitialState(
            msg->usage & D3DUSAGE_RENDERTARGET ? nvrhi::ResourceStates::RenderTarget :
            msg->usage & D3DUSAGE_DEPTHSTENCIL ? nvrhi::ResourceStates::DepthWrite :
            nvrhi::ResourceStates::CopyDest)
        .setKeepInitialState(true)
        .setFormat(format)
        .setIsTypeless(isRenderTargetOrDepthStencil)
        .setClearValue(msg->usage & D3DUSAGE_DEPTHSTENCIL ? nvrhi::Color(1, 0, 0, 0) : nvrhi::Color(0))
        .setUseClearValue(isRenderTargetOrDepthStencil));
}

static void createBuffer(
    const Bridge& bridge,
    size_t length,
    D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_STATES initialState,
    D3D12MA::Allocation** allocation)
{
    D3D12MA::ALLOCATION_DESC allocationDesc{};
    allocationDesc.HeapType = heapType;

    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(length);

    const HRESULT result = bridge.device.allocator->CreateResource(
        &allocationDesc,
        &resourceDesc,
        initialState,
        nullptr,
        allocation,
        IID_ID3D12Resource,
        nullptr);

#ifndef _DEBUG
    if (result != S_OK)
    {
        char text[1024]{};
        sprintf(text,
            "Allocation Failed!\n"
            "length: %d\n"
            "heapType: %d\n"
            "initialState: %d\n"
            "HRESULT: %x", length, heapType, initialState, result);

        MessageBoxA(nullptr, text, nullptr, MB_ICONERROR);
    }
#else
    assert(result == S_OK && *allocation && (*allocation)->GetResource());
#endif
}

static void createBuffer(
    const Bridge& bridge,
    size_t length,
    nvrhi::Format format,
    nvrhi::BufferHandle& buffer,
    D3D12MA::Allocation** allocation,
    bool isVertexBuffer,
    bool isIndexBuffer,
    bool isAccelStructBuildInput,
    bool isCpuWrite)
{
    createBuffer(
        bridge, 
        length, 
        isCpuWrite ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT,
        isCpuWrite ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST,
        allocation);

    buffer = bridge.device.nvrhi->createHandleForNativeBuffer(nvrhi::ObjectTypes::D3D12_Resource,
        (*allocation)->GetResource(),
        nvrhi::BufferDesc()
        .setByteSize(length)
        .setFormat(format)
        .setIsVertexBuffer(isVertexBuffer)
        .setIsIndexBuffer(isIndexBuffer)
        .setIsAccelStructBuildInput(isAccelStructBuildInput)
        .setInitialState(nvrhi::ResourceStates::CopyDest)
        .setKeepInitialState(!isCpuWrite)
        .setCpuAccess(isCpuWrite ? nvrhi::CpuAccessMode::Write : nvrhi::CpuAccessMode::None)
        .setCanHaveTypedViews(format != nvrhi::Format::UNKNOWN)
        .setCanHaveRawViews(format == nvrhi::Format::UNKNOWN));
}

void Bridge::procMsgCreateVertexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexBuffer>();

    createBuffer(
        *this,
        msg->length,
        nvrhi::Format::UNKNOWN,
        reinterpret_cast<nvrhi::BufferHandle&>(resources[msg->vertexBuffer]),
        allocations[msg->vertexBuffer].ReleaseAndGetAddressOf(),
        true,
        false,
        true,
        false);
}

void Bridge::procMsgCreateIndexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateIndexBuffer>();

    createBuffer(
        *this,
        msg->length,
        Format::convert(msg->format),
        reinterpret_cast<nvrhi::BufferHandle&>(resources[msg->indexBuffer]),
        allocations[msg->indexBuffer].ReleaseAndGetAddressOf(),
        false,
        true,
        true,
        false);
}

void Bridge::procMsgCreateRenderTarget()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateRenderTarget>();
}

void Bridge::procMsgCreateDepthStencilSurface()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateDepthStencilSurface>();
}

void Bridge::procMsgSetRenderTarget()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetRenderTarget>();
    assert(msg->index == 0);
    const auto& texture = msg->surface == swapChainSurface ? swapChainTextures[swapChain->GetCurrentBackBufferIndex()] : resources[msg->surface];

    if (texture)
    {
        if (framebufferDesc.colorAttachments.empty())
        {
            framebufferDesc.colorAttachments.emplace_back();
            dirtyFlags |= DirtyFlags::FramebufferAndPipeline;
        }

        assignAndUpdateDirtyFlags(
            framebufferDesc.colorAttachments[0].texture,
            nvrhi::checked_cast<nvrhi::ITexture*>(texture.Get()), 
            DirtyFlags::FramebufferAndPipeline);

        renderTargets[msg->index] = framebufferDesc.colorAttachments[msg->index].texture;
    }
    else
    {
        if (!framebufferDesc.colorAttachments.empty())
        {
            framebufferDesc.colorAttachments[0].texture = nullptr;
            framebufferDesc.colorAttachments.pop_back();
            dirtyFlags |= DirtyFlags::FramebufferAndPipeline;
        }

        renderTargets[msg->index] = nullptr;
    }
}

void Bridge::procMsgSetDepthStencilSurface()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetDepthStencilSurface>();

    assignAndUpdateDirtyFlags(
        framebufferDesc.depthAttachment.texture,
        nvrhi::checked_cast<nvrhi::ITexture*>(resources[msg->surface].Get()),
        DirtyFlags::FramebufferAndPipeline);

    depthStencil = framebufferDesc.depthAttachment.texture;
}

void Bridge::procMsgClear()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgClear>();

    if (msg->flags & D3DCLEAR_TARGET && !framebufferDesc.colorAttachments.empty())
    {
        commandList->clearTextureFloat(
            framebufferDesc.colorAttachments[0].texture,
            nvrhi::TextureSubresourceSet(),
            nvrhi::Color(
                ((msg->color >> 16) & 0xFF) / 255.0f,
                ((msg->color >> 8) & 0xFF) / 255.0f,
                (msg->color & 0xFF) / 255.0f,
                ((msg->color >> 24) & 0xFF) / 255.0f));
    }

    if (msg->flags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) && framebufferDesc.depthAttachment.texture)
    {
        commandList->clearDepthStencilTexture(
            framebufferDesc.depthAttachment.texture,
            nvrhi::TextureSubresourceSet(),
            msg->flags & D3DCLEAR_ZBUFFER,
            msg->z,
            msg->flags & D3DCLEAR_STENCIL,
            (uint8_t)msg->stencil);
    }
}

void Bridge::procMsgSetViewport()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetViewport>();

    if (graphicsState.viewport.viewports.empty())
    {
        graphicsState.viewport.viewports.emplace_back();
        dirtyFlags |= DirtyFlags::GraphicsState;
    }

    auto& viewport = graphicsState.viewport.viewports[0];

    assignAndUpdateDirtyFlags(viewport.minX, (float)msg->x, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(viewport.maxX, (float)(msg->x + msg->width), DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(viewport.minY, (float)msg->y, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(viewport.maxY, (float)(msg->y + msg->height), DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(viewport.minZ, msg->minZ, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(viewport.maxZ, msg->maxZ, DirtyFlags::GraphicsState);
}

void Bridge::procMsgSetRenderState()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetRenderState>();

    switch (msg->state)
    {
    case D3DRS_ZENABLE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.depthStencilState.depthTestEnable, 
            (bool)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_FILLMODE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.rasterState.fillMode,
            msg->value == D3DFILL_WIREFRAME ? nvrhi::RasterFillMode::Wireframe :
            nvrhi::RasterFillMode::Solid,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_ZWRITEENABLE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.depthStencilState.depthWriteEnable,
            (bool)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_ALPHATESTENABLE:
        assignAndUpdateDirtyFlags(
            sharedConstants.flags,
            sharedConstants.flags & ~SHARED_FLAGS_ENABLE_ALPHA_TEST | (msg->value ? SHARED_FLAGS_ENABLE_ALPHA_TEST : 0),
            DirtyFlags::SharedConstants);

        break;

    case D3DRS_SRCBLEND:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].srcBlend,
            (nvrhi::BlendFactor)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_DESTBLEND:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].destBlend, 
            (nvrhi::BlendFactor)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_CULLMODE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.rasterState.cullMode,

            msg->value == D3DCULL_CW ? nvrhi::RasterCullMode::Front :
            msg->value == D3DCULL_CCW ? nvrhi::RasterCullMode::Back :
            nvrhi::RasterCullMode::None,

            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_ZFUNC:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.depthStencilState.depthFunc,
            (nvrhi::ComparisonFunc)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_ALPHAREF:
        assignAndUpdateDirtyFlags(
            sharedConstants.alphaThreshold,
            (float)msg->value / 255.0f,
            DirtyFlags::SharedConstants);

        break;

    case D3DRS_ALPHABLENDENABLE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].blendEnable,
            (bool)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_COLORWRITEENABLE:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].colorWriteMask, 
            (nvrhi::ColorMask)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_BLENDOP:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].blendOp, 
            (nvrhi::BlendOp)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_SLOPESCALEDEPTHBIAS:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.rasterState.slopeScaledDepthBias,
            *(float*)&msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_COLORWRITEENABLE1:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[1].colorWriteMask,
            (nvrhi::ColorMask)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_COLORWRITEENABLE2:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[2].colorWriteMask, 
            (nvrhi::ColorMask)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_COLORWRITEENABLE3:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[3].colorWriteMask, 
            (nvrhi::ColorMask)msg->value, DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_DEPTHBIAS:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.rasterState.depthBias, 
            (int)(*(float*)&msg->value * (1 << 24)), 
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_SRCBLENDALPHA:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].srcBlendAlpha, 
            (nvrhi::BlendFactor)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_DESTBLENDALPHA:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].destBlendAlpha, 
            (nvrhi::BlendFactor)msg->value,
            DirtyFlags::FramebufferAndPipeline);

        break;

    case D3DRS_BLENDOPALPHA:
        assignAndUpdateDirtyFlags(
            pipelineDesc.renderState.blendState.targets[0].blendOpAlpha,
            (nvrhi::BlendOp)msg->value, 
            DirtyFlags::FramebufferAndPipeline);

        break;
    }
}

void Bridge::procMsgSetTexture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetTexture>();
    const auto& texture = resources[msg->texture];

    assignAndUpdateDirtyFlags(
        textureBindingSetDesc.bindings[msg->stage], 
        nvrhi::BindingSetItem::Texture_SRV(msg->stage, textures[msg->stage] = texture ? nvrhi::checked_cast<nvrhi::ITexture*>(texture.Get()) : nullTexture.Get()),
        DirtyFlags::Texture);

    textureAllocations[msg->stage] = allocations[msg->texture];

    if (msg->stage == 7 || msg->stage == 13)
    {
        assignAndUpdateDirtyFlags(
            samplerDescs[msg->stage].reductionType,
            nvrhi::SamplerReductionType::Comparison,
            DirtyFlags::Sampler);
    }
}

void Bridge::procMsgSetSamplerState()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetSamplerState>();

    constexpr nvrhi::SamplerAddressMode addressMap[] =
    {
        (nvrhi::SamplerAddressMode)-1,
        nvrhi::SamplerAddressMode::Wrap,
        nvrhi::SamplerAddressMode::Mirror,
        nvrhi::SamplerAddressMode::Clamp,
        nvrhi::SamplerAddressMode::Border,
        nvrhi::SamplerAddressMode::MirrorOnce
    };

    switch (msg->type)
    {
    case D3DSAMP_ADDRESSU:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].addressU, addressMap[msg->value], DirtyFlags::Sampler);
        break;

    case D3DSAMP_ADDRESSV:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].addressV, addressMap[msg->value], DirtyFlags::Sampler);
        break;

    case D3DSAMP_ADDRESSW:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].addressW, addressMap[msg->value], DirtyFlags::Sampler);
        break;

    case D3DSAMP_BORDERCOLOR:
        break;

    case D3DSAMP_MAGFILTER:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].magFilter, msg->value == D3DTEXF_LINEAR, DirtyFlags::Sampler);
        break;

    case D3DSAMP_MINFILTER:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].minFilter, msg->value == D3DTEXF_LINEAR, DirtyFlags::Sampler);
        break;

    case D3DSAMP_MIPFILTER:
        assignAndUpdateDirtyFlags(samplerDescs[msg->sampler].mipFilter, msg->value == D3DTEXF_LINEAR, DirtyFlags::Sampler);
        break;
    }
}

void Bridge::procMsgSetScissorRect()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetScissorRect>();

    if (graphicsState.viewport.scissorRects.empty())
    {
        graphicsState.viewport.scissorRects.emplace_back();
        dirtyFlags |= DirtyFlags::GraphicsState;
    }

    auto& rect = graphicsState.viewport.scissorRects[0];

    assignAndUpdateDirtyFlags(rect.minX, msg->left, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(rect.maxX, msg->right, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(rect.minY, msg->top, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(rect.maxY, msg->bottom, DirtyFlags::GraphicsState);
}

void Bridge::procMsgDrawPrimitive()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawPrimitive>();

    assignAndUpdateDirtyFlags(
        pipelineDesc.primType, 
        Format::convertPrimitiveType(msg->primitiveType), 
        DirtyFlags::FramebufferAndPipeline);

    processDirtyFlags();

    commandList->draw(nvrhi::DrawArguments()
        .setStartVertexLocation(msg->startVertex)
        .setVertexCount(msg->primitiveCount)
        .setInstanceCount(instancing ? instanceCount : 1));
}

void Bridge::procMsgDrawIndexedPrimitive()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawIndexedPrimitive>();

    assignAndUpdateDirtyFlags(
        pipelineDesc.primType,
        Format::convertPrimitiveType(msg->primitiveType),
        DirtyFlags::FramebufferAndPipeline);

    processDirtyFlags();

    assert(graphicsState.indexBuffer.buffer);

    commandList->drawIndexed(nvrhi::DrawArguments()
        .setStartVertexLocation(msg->baseVertexIndex)
        .setStartIndexLocation(msg->startIndex)
        .setVertexCount(msg->primitiveCount)
        .setInstanceCount(instancing ? instanceCount : 1));
}

void Bridge::procMsgDrawPrimitiveUP()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawPrimitiveUP>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vertexStreamZeroSize);

    assignAndUpdateDirtyFlags(
        pipelineDesc.primType,
        Format::convertPrimitiveType(msg->primitiveType),
        DirtyFlags::FramebufferAndPipeline);

    auto& vertexBuffer = vertexBuffers[XXH64(data, msg->vertexStreamZeroSize, 0)];

    if (!vertexBuffer)
    {
        createBuffer(
            *this,
            msg->vertexStreamZeroSize,
            nvrhi::Format::UNKNOWN,
            vertexBuffer,
            vertexBufferAllocations.emplace_back().GetAddressOf(),
            true,
            false,
            false,
            true);

        void* destCopy = device.nvrhi->mapBuffer(vertexBuffer, nvrhi::CpuAccessMode::Write);
        memcpy(destCopy, data, msg->vertexStreamZeroSize);
        device.nvrhi->unmapBuffer(vertexBuffer);
    }

    if (graphicsState.vertexBuffers.empty())
    {
        graphicsState.vertexBuffers.emplace_back();
        dirtyFlags |= DirtyFlags::VertexBuffer;
    }

    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].buffer, vertexBuffer, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].slot, 0, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].offset, 0, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(vertexStrides[0], msg->vertexStreamZeroStride, DirtyFlags::InputLayout);

    processDirtyFlags();

    commandList->draw(nvrhi::DrawArguments()
        .setVertexCount(msg->primitiveCount)
        .setInstanceCount(instancing ? instanceCount : 1));
}

void Bridge::procMsgCreateVertexDeclaration()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexDeclaration>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vertexElementCount * sizeof(D3DVERTEXELEMENT9));

    auto& attributes = vertexAttributeDescs[msg->vertexDeclaration];

    for (size_t i = 0; i < msg->vertexElementCount; i++)
    {
        auto& element = ((D3DVERTEXELEMENT9*)data)[i];

        std::string name = Format::convertDeclUsage(element.Usage);
        if (element.UsageIndex > 0)
        {
            name += std::to_string(element.UsageIndex);
            name += "N";
        }

        attributes.emplace_back()
            .setName(name)
            .setFormat(Format::convertDeclType(element.Type))
            .setBufferIndex(element.Stream)
            .setOffset(element.Offset);
    }

    constexpr std::pair<const char*, nvrhi::Format> ATTRIBUTE_DESC[] =
    {
        { "POSITION", nvrhi::Format::R8_UNORM },
        { "NORMAL", nvrhi::Format::R8_UNORM },
        { "TANGENT", nvrhi::Format::R8_UNORM },
        { "BINORMAL", nvrhi::Format::R8_UNORM },
        { "TEXCOORD", nvrhi::Format::R8_UNORM },
        { "TEXCOORD1N", nvrhi::Format::R8_UNORM },
        { "TEXCOORD2N", nvrhi::Format::R8_UNORM },
        { "TEXCOORD3N", nvrhi::Format::R8_UNORM },
        { "COLOR", nvrhi::Format::R8_UNORM },
        { "COLOR1N", nvrhi::Format::R8_UNORM },
        { "BLENDWEIGHT", nvrhi::Format::R8_UNORM },
        { "BLENDINDICES", nvrhi::Format::R8_UINT }
    };

    for (auto& desc : ATTRIBUTE_DESC)
    {
        bool found = false;

        for (auto& attribute : attributes)
        {
            found = attribute.name == desc.first;
            if (found)
                break;
        }

        if (!found)
        {
            attributes.push_back(nvrhi::VertexAttributeDesc()
                .setName(desc.first)
                .setFormat(desc.second));
        }
    }
}

void Bridge::procMsgSetVertexDeclaration()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexDeclaration>();

    assignAndUpdateDirtyFlags(vertexDeclaration, msg->vertexDeclaration, DirtyFlags::InputLayout);

    if (fvf)
    {
        fvf = false;
        dirtyFlags |= DirtyFlags::FramebufferAndPipeline;
    }
}

void Bridge::procMsgCreateVertexShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);

    auto& shader = shaders[XXH64(data, msg->functionSize, 0)];
    if (!shader)
        shader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), data, msg->functionSize);

    resources[msg->shader] = shader;
}

void Bridge::procMsgSetVertexShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShader>();
    const auto& shader = resources[msg->shader];
    if (shader)
        assignAndUpdateDirtyFlags(pipelineDesc.VS, nvrhi::checked_cast<nvrhi::IShader*>(shader.Get()), DirtyFlags::FramebufferAndPipeline);
}

void Bridge::procMsgSetVertexShaderConstantF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShaderConstantF>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vector4fCount * sizeof(FLOAT[4]));

    vsConstants.writeConstant(msg->startRegister, (float*)data, msg->vector4fCount);
    dirtyFlags |= DirtyFlags::VsConstants;
}

void Bridge::procMsgSetVertexShaderConstantB()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShaderConstantB>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->boolCount * sizeof(BOOL));

    const uint32_t mask = 1 << msg->startRegister;
    const uint32_t value = *(const BOOL*)data << msg->startRegister;

    assignAndUpdateDirtyFlags(sharedConstants.booleans, (sharedConstants.booleans & ~mask) | value, DirtyFlags::SharedConstants);
}

void Bridge::procMsgSetStreamSource()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetStreamSource>();

    while (graphicsState.vertexBuffers.size() < msg->streamNumber + 1)
    {
        graphicsState.vertexBuffers.emplace_back();
        dirtyFlags |= DirtyFlags::VertexBuffer;
    }

    auto& vertexBuffer = graphicsState.vertexBuffers[msg->streamNumber];

    assignAndUpdateDirtyFlags(vertexBuffer.slot, msg->streamNumber, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(vertexBuffer.buffer, nvrhi::checked_cast<nvrhi::IBuffer*>(resources[msg->streamData].Get()), DirtyFlags::VertexBuffer | DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(vertexBuffer.offset, msg->offsetInBytes, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(vertexStrides[msg->streamNumber], msg->stride, DirtyFlags::InputLayout);

    streamSources[msg->streamNumber] = graphicsState.vertexBuffers[msg->streamNumber].buffer;
    streamSourceAllocations[msg->streamNumber] = allocations[msg->streamData];
}

void Bridge::procMsgSetStreamSourceFreq()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetStreamSourceFreq>();

    if (msg->streamNumber != 0)
        return;

    assignAndUpdateDirtyFlags(instancing, (msg->setting & D3DSTREAMSOURCE_INDEXEDDATA) != 0, DirtyFlags::InputLayout);
    instanceCount = msg->setting & 0x3FFFFFFF;
}

void Bridge::procMsgSetIndices()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetIndices>();
    auto& indexBuffer = graphicsState.indexBuffer;

    assignAndUpdateDirtyFlags(indexBuffer.buffer, nvrhi::checked_cast<nvrhi::IBuffer*>(resources[msg->indexData].Get()), DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(indexBuffer.offset, 0, DirtyFlags::GraphicsState);

    if (indexBuffer.buffer)
        assignAndUpdateDirtyFlags(indexBuffer.format, indexBuffer.buffer->getDesc().format, DirtyFlags::GraphicsState);

    indices = indexBuffer.buffer;
    indicesAllocation = allocations[msg->indexData];
}

void Bridge::procMsgCreatePixelShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreatePixelShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);

    auto& shader = shaders[XXH64(data, msg->functionSize, 0)];
    if (!shader)
        shader = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), data, msg->functionSize);

    resources[msg->shader] = shader;
}

void Bridge::procMsgSetPixelShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShader>();
    assignAndUpdateDirtyFlags(pipelineDesc.PS, nvrhi::checked_cast<nvrhi::IShader*>(resources[msg->shader].Get()), DirtyFlags::FramebufferAndPipeline);
}

void Bridge::procMsgSetPixelShaderConstantF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShaderConstantF>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vector4fCount * sizeof(FLOAT[4]));

    psConstants.writeConstant(msg->startRegister, (float*)data, msg->vector4fCount);
    dirtyFlags |= DirtyFlags::PsConstants;
}

void Bridge::procMsgSetPixelShaderConstantB()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShaderConstantB>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->boolCount * sizeof(BOOL));

    const uint32_t mask = 1 << (16 + msg->startRegister);
    const uint32_t value = *(const bool*)data << (16 + msg->startRegister);

    assignAndUpdateDirtyFlags(sharedConstants.booleans, (sharedConstants.booleans & ~mask) | value, DirtyFlags::SharedConstants);
}

void Bridge::procMsgMakePicture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgMakePicture>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);

    nvrhi::TextureHandle texture;
    std::vector<D3D12_SUBRESOURCE_DATA> subResources;

    DirectX::LoadDDSTextureFromMemory(
        device.nvrhi,
        device.allocator.Get(),
        (const uint8_t*)data,
        msg->size,
        std::addressof(texture),
        allocations[msg->texture].ReleaseAndGetAddressOf(),
        nullptr,
        subResources);

    if (!texture)
    {
        texture = nullTexture;
        return;
    }

    openCommandListForCopy();

    for (uint32_t i = 0; i < subResources.size(); i++)
    {
        const auto& subResource = subResources[i];

        commandListForCopy->writeTexture(
            texture,
            i / texture->getDesc().mipLevels,
            i % texture->getDesc().mipLevels,
            subResource.pData,
            subResource.RowPitch,
            subResource.SlicePitch);
    }

    commandList->setPermanentTextureState(texture, nvrhi::ResourceStates::ShaderResource);

    static_cast<ID3D12Resource*>(texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource))->SetName(msg->name);

    resources[msg->texture] = texture;
}

void Bridge::procMsgWriteBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgWriteBuffer>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);

    const auto buffer = nvrhi::checked_cast<nvrhi::IBuffer*>(resources[msg->buffer].Get());
    if (!buffer || msg->size == 0)
        return;

    openCommandListForCopy();

    commandListForCopy->writeBuffer(
        buffer,
        data,
        min(msg->size, buffer->getDesc().byteSize));
}

void Bridge::procMsgWriteTexture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgWriteTexture>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);
}

void Bridge::procMsgExit()
{
    msgReceiver.getMsgAndMoveNext<MsgExit>();
    shouldExit = true;
}

void Bridge::procMsgReleaseResource()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgReleaseResource>();
    pendingReleases.push_back(msg->resource);
}

void Bridge::processMessages()
{
    while (msgReceiver.hasNext())
    {
        switch (msgReceiver.getMsgId())
        {
        case MsgSetFVF::ID:                    procMsgSetFVF(); break;
        case MsgInitSwapChain::ID:             procMsgInitSwapChain(); break;
        case MsgPresent::ID:                   procMsgPresent(); break;
        case MsgCreateTexture::ID:             procMsgCreateTexture(); break;
        case MsgCreateVertexBuffer::ID:        procMsgCreateVertexBuffer(); break;
        case MsgCreateIndexBuffer::ID:         procMsgCreateIndexBuffer(); break;
        case MsgCreateRenderTarget::ID:        procMsgCreateRenderTarget(); break;
        case MsgCreateDepthStencilSurface::ID: procMsgCreateDepthStencilSurface(); break;
        case MsgSetRenderTarget::ID:           procMsgSetRenderTarget(); break;
        case MsgSetDepthStencilSurface::ID:    procMsgSetDepthStencilSurface(); break;
        case MsgClear::ID:                     procMsgClear(); break;
        case MsgSetViewport::ID:               procMsgSetViewport(); break;
        case MsgSetRenderState::ID:            procMsgSetRenderState(); break;
        case MsgSetTexture::ID:                procMsgSetTexture(); break;
        case MsgSetSamplerState::ID:           procMsgSetSamplerState(); break;
        case MsgSetScissorRect::ID:            procMsgSetScissorRect(); break;
        case MsgDrawPrimitive::ID:             procMsgDrawPrimitive(); break;
        case MsgDrawIndexedPrimitive::ID:      procMsgDrawIndexedPrimitive(); break;
        case MsgDrawPrimitiveUP::ID:           procMsgDrawPrimitiveUP(); break;
        case MsgCreateVertexDeclaration::ID:   procMsgCreateVertexDeclaration(); break;
        case MsgSetVertexDeclaration::ID:      procMsgSetVertexDeclaration(); break;
        case MsgCreateVertexShader::ID:        procMsgCreateVertexShader(); break;
        case MsgSetVertexShader::ID:           procMsgSetVertexShader(); break;
        case MsgSetVertexShaderConstantF::ID:  procMsgSetVertexShaderConstantF(); break;
        case MsgSetVertexShaderConstantB::ID:  procMsgSetVertexShaderConstantB(); break;
        case MsgSetStreamSource::ID:           procMsgSetStreamSource(); break;
        case MsgSetStreamSourceFreq::ID:       procMsgSetStreamSourceFreq(); break;
        case MsgSetIndices::ID:                procMsgSetIndices(); break;
        case MsgCreatePixelShader::ID:         procMsgCreatePixelShader(); break;
        case MsgSetPixelShader::ID:            procMsgSetPixelShader(); break;
        case MsgSetPixelShaderConstantF::ID:   procMsgSetPixelShaderConstantF(); break;
        case MsgSetPixelShaderConstantB::ID:   procMsgSetPixelShaderConstantB(); break;
        case MsgMakePicture::ID:               procMsgMakePicture(); break;
        case MsgWriteBuffer::ID:               procMsgWriteBuffer(); break;
        case MsgWriteTexture::ID:              procMsgWriteTexture(); break;
        case MsgCreateGeometry::ID:            raytracing.procMsgCreateGeometry(*this); break;
        case MsgCreateBottomLevelAS::ID:       raytracing.procMsgCreateBottomLevelAS(*this); break;
        case MsgCreateInstance::ID:            raytracing.procMsgCreateInstance(*this); break;
        case MsgNotifySceneTraversed::ID:      raytracing.procMsgNotifySceneTraversed(*this); break;
        case MsgCreateMaterial::ID:            raytracing.procMsgCreateMaterial(*this); break;
        case MsgExit::ID:                      procMsgExit(); break;
        case MsgReleaseResource::ID:           procMsgReleaseResource(); break;
        default:                               assert(0 && "Unknown message type"); break;
        }
    }
}

void Bridge::receiveMessages()
{
    while (!shouldExit)
    {
        openCommandList();
        processMessages();
        closeAndExecuteCommandLists();

        if (shouldPresent)
        {
            swapChain->Present(0, 0);
            shouldPresent = false;
        }

        textureBindingSets.clear();
        samplerBindingSets.clear();
        vertexBuffers.clear();
        vertexBufferAllocations.clear();

        for (const auto resource : pendingReleases)
        {
            vertexAttributeDescs.erase(resource);
            raytracing.bottomLevelAccelStructs.erase(resource);
            raytracing.materials.erase(resource);
            resources.erase(resource);
            allocations.erase(resource);
        }

        pendingReleases.clear();
    }
}
