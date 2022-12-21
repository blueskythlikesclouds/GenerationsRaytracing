#include "Bridge.h"

#include "Format.h"
#include "Hash.h"
#include "Message.h"

Bridge::Bridge()
{
    commandList = device.nvrhi->createCommandList();

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
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(0, vsConstantBuffer))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, sharedConstantBuffer)), vsBindingLayout);

    psBindingLayout = device.nvrhi->createBindingLayout(nvrhi::BindingLayoutDesc()
        .setVisibility(nvrhi::ShaderType::Pixel)
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(0))
        .addItem(nvrhi::BindingLayoutItem::VolatileConstantBuffer(1)));

    psBindingSet = device.nvrhi->createBindingSet(nvrhi::BindingSetDesc()
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
    
    framebufferDesc.colorAttachments.emplace_back();

    pipelineDesc.bindingLayouts.push_back(vsBindingLayout);
    pipelineDesc.bindingLayouts.push_back(psBindingLayout);
    pipelineDesc.bindingLayouts.push_back(textureBindingLayout);
    pipelineDesc.bindingLayouts.push_back(samplerBindingLayout);

    graphicsState.viewport.viewports.emplace_back();
    graphicsState.viewport.scissorRects.emplace_back();

    graphicsState.bindings.push_back(vsBindingSet);
    graphicsState.bindings.push_back(psBindingSet);
    graphicsState.bindings.emplace_back();
    graphicsState.bindings.emplace_back();
}

void Bridge::openCommandList()
{
    assert(!openedCommandList);

    commandList->open();
    openedCommandList = true;

    dirtyFlags = DirtyFlags::All;
}

void Bridge::closeAndExecuteCommandList()
{
    if (!openedCommandList)
        return;

    commandList->close();
    openedCommandList = false;

    device.nvrhi->executeCommandList(commandList);
    device.nvrhi->waitForIdle();
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

    if ((dirtyFlags & DirtyFlags::Texture) != DirtyFlags::None)
    {
        auto& textureBindingSet = textureBindingSets[hash = computeHash(textureBindingSetDesc.bindings.data(), textureBindingSetDesc.bindings.size(), 0)];
        if (!textureBindingSet)
            textureBindingSet = device.nvrhi->createBindingSet(textureBindingSetDesc, textureBindingLayout);

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

    auto framebufferDesc = this->framebufferDesc;
    auto pipelineDesc = this->pipelineDesc;

    if (!framebufferDesc.colorAttachments.empty() && framebufferDesc.depthAttachment.texture)
    {
        auto& textureDesc = framebufferDesc.colorAttachments[0].texture->getDesc();

        if (textureDesc.width == 320 && textureDesc.height == 180)
            assignAndUpdateDirtyFlags(framebufferDesc.depthAttachment.texture, nullptr, DirtyFlags::FramebufferAndPipeline);
    }
    if (!framebufferDesc.depthAttachment.texture &&
        (pipelineDesc.renderState.depthStencilState.depthTestEnable ||
            pipelineDesc.renderState.depthStencilState.depthWriteEnable))
    {
        assignAndUpdateDirtyFlags(pipelineDesc.renderState.depthStencilState.depthTestEnable, false, DirtyFlags::FramebufferAndPipeline);
        assignAndUpdateDirtyFlags(pipelineDesc.renderState.depthStencilState.depthWriteEnable, false, DirtyFlags::FramebufferAndPipeline);
    }

    if ((dirtyFlags & DirtyFlags::FramebufferAndPipeline) != DirtyFlags::None)
    {
        hash = computeHash(framebufferDesc.colorAttachments.data(), framebufferDesc.colorAttachments.size(), 0);
        hash = computeHash(framebufferDesc.depthAttachment, hash);

        auto& framebuffer = framebuffers[hash];
        if (!framebuffer)
            framebuffer = device.nvrhi->createFramebuffer(framebufferDesc);

        hash = computeHash(pipelineDesc, hash);
        auto& pipeline = pipelines[hash];
        if (!pipeline)
            pipeline = device.nvrhi->createGraphicsPipeline(pipelineDesc, framebuffer);

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

    auto& attributes = vertexAttributeDescs[msg->fvf];
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

    assignAndUpdateDirtyFlags(vertexDeclaration, msg->fvf, DirtyFlags::InputLayout);
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

    resources[msg->texture] = device.nvrhi->createTexture(nvrhi::TextureDesc()
        .setWidth(msg->width)
        .setHeight(msg->height)
        .setMipLevels(msg->levels)
        .setIsRenderTarget(msg->usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
        .setInitialState(
            msg->usage & D3DUSAGE_RENDERTARGET ? nvrhi::ResourceStates::RenderTarget :
            msg->usage & D3DUSAGE_DEPTHSTENCIL ? nvrhi::ResourceStates::DepthWrite :
            nvrhi::ResourceStates::ShaderResource)
        .setKeepInitialState(true)
        .setFormat(format)
        .setIsTypeless(msg->usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)));
}

void Bridge::procMsgCreateVertexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexBuffer>();

    resources[msg->vertexBuffer] = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(msg->length)
        .setIsVertexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::VertexBuffer)
        .setKeepInitialState(true));
}

void Bridge::procMsgCreateIndexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateIndexBuffer>();

    resources[msg->indexBuffer] = device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(msg->length)
        .setFormat(Format::convert(msg->format))
        .setIsIndexBuffer(true)
        .setInitialState(nvrhi::ResourceStates::IndexBuffer)
        .setKeepInitialState(true));
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
    }
    else
    {
        if (!framebufferDesc.colorAttachments.empty())
        {
            framebufferDesc.colorAttachments[0].texture = nullptr;
            framebufferDesc.colorAttachments.pop_back();
            dirtyFlags |= DirtyFlags::FramebufferAndPipeline;
        }
    }
}

void Bridge::procMsgSetDepthStencilSurface()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetDepthStencilSurface>();

    assignAndUpdateDirtyFlags(
        framebufferDesc.depthAttachment.texture,
        nvrhi::checked_cast<nvrhi::ITexture*>(resources[msg->surface].Get()),
        DirtyFlags::FramebufferAndPipeline);
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
        nvrhi::BindingSetItem::Texture_SRV(msg->stage, texture ? nvrhi::checked_cast<nvrhi::ITexture*>(texture.Get()) : nullTexture.Get()),
        DirtyFlags::Texture);

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

    auto& vertexBuffer = vertexBuffers[msg->vertexStreamZeroSize];

    if (!vertexBuffer)
    {
        vertexBuffer = device.nvrhi->createBuffer(nvrhi::BufferDesc()
            .setByteSize(msg->vertexStreamZeroSize)
            .setIsVertexBuffer(true)
            .setInitialState(nvrhi::ResourceStates::VertexBuffer)
            .setKeepInitialState(true));
    }

    if (graphicsState.vertexBuffers.empty())
    {
        graphicsState.vertexBuffers.emplace_back();
        dirtyFlags |= DirtyFlags::VertexBuffer;
    }

    commandList->writeBuffer(vertexBuffer, data, msg->vertexStreamZeroSize);
    commandList->setBufferState(vertexBuffer, nvrhi::ResourceStates::VertexBuffer);

    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].buffer, vertexBuffer, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].slot, 0, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(graphicsState.vertexBuffers[0].offset, 0, DirtyFlags::GraphicsState);
    assignAndUpdateDirtyFlags(vertexStrides[0], msg->vertexStreamZeroStride, DirtyFlags::InputLayout);

    processDirtyFlags();

    commandList->draw(nvrhi::DrawArguments()
        .setVertexCount(msg->primitiveCount)
        .setInstanceCount(instancing ? instanceCount : 1));
}

static void addVertexAttributeDesc(const char* name, nvrhi::Format format, std::vector<nvrhi::VertexAttributeDesc>& attributes)
{
    bool found = false;

    for (auto& attr : attributes)
    {
        found = attr.name == name;
        if (found)
            break;
    }

    if (!found)
    {
        attributes.push_back(nvrhi::VertexAttributeDesc()
            .setName(name)
            .setFormat(format));
    }
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
            switch (element.UsageIndex)
            {
            case 1: name += "_ONE"; break;
            case 2: name += "_TWO"; break;
            case 3: name += "_THREE"; break;
            }
        }

        attributes.emplace_back()
            .setName(name)
            .setFormat(Format::convertDeclType(element.Type))
            .setBufferIndex(element.Stream)
            .setOffset(element.Offset);
    }

    addVertexAttributeDesc("POSITION", nvrhi::Format::RGB32_FLOAT, attributes);
    addVertexAttributeDesc("NORMAL", nvrhi::Format::RGB32_FLOAT, attributes);
    addVertexAttributeDesc("TANGENT", nvrhi::Format::RGB32_FLOAT, attributes);
    addVertexAttributeDesc("BINORMAL", nvrhi::Format::RGB32_FLOAT, attributes);
    addVertexAttributeDesc("TEXCOORD", nvrhi::Format::RG32_FLOAT, attributes);
    addVertexAttributeDesc("TEXCOORD_ONE", nvrhi::Format::RG32_FLOAT, attributes);
    addVertexAttributeDesc("TEXCOORD_TWO", nvrhi::Format::RG32_FLOAT, attributes);
    addVertexAttributeDesc("TEXCOORD_THREE", nvrhi::Format::RG32_FLOAT, attributes);
    addVertexAttributeDesc("COLOR", nvrhi::Format::BGRA8_UNORM, attributes);
    addVertexAttributeDesc("COLOR_ONE", nvrhi::Format::BGRA8_UNORM, attributes);
    addVertexAttributeDesc("BLENDWEIGHT", nvrhi::Format::RGBA8_UNORM, attributes);
    addVertexAttributeDesc("BLENDINDICES", nvrhi::Format::RGBA8_UINT, attributes);
}

void Bridge::procMsgSetVertexDeclaration()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexDeclaration>();

    assignAndUpdateDirtyFlags(vertexDeclaration, msg->vertexDeclaration, DirtyFlags::InputLayout);
}

void Bridge::procMsgCreateVertexShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);

    resources[msg->shader] = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), data, msg->functionSize);
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
}

void Bridge::procMsgCreatePixelShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreatePixelShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);

    resources[msg->shader] = device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), data, msg->functionSize);
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
        nullptr,
        (const uint8_t*)data,
        msg->size,
        std::addressof(texture),
        nullptr,
        nullptr,
        subResources);

    for (uint32_t i = 0; i < subResources.size(); i++)
    {
        const auto& subResource = subResources[i];

        commandList->writeTexture(
            texture,
            i / texture->getDesc().mipLevels,
            i % texture->getDesc().mipLevels,
            subResource.pData,
            subResource.RowPitch,
            subResource.SlicePitch);
    }

    commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(), texture->getDesc().initialState);

    resources[msg->texture] = texture;
}

void Bridge::procMsgWriteBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgWriteBuffer>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);

    const auto buffer = nvrhi::checked_cast<nvrhi::IBuffer*>(resources[msg->buffer].Get());

    assert(msg->size);

    commandList->writeBuffer(
        buffer,
        data,
        msg->size);

    commandList->setBufferState(buffer, buffer->getDesc().initialState);
}

void Bridge::procMsgExit()
{
    msgReceiver.getMsgAndMoveNext<MsgExit>();
    shouldExit = true;
}

void Bridge::procMsgReleaseResource()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgReleaseResource>();

    resources.erase(msg->resource);
    vertexAttributeDescs.erase(msg->resource);
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
        closeAndExecuteCommandList();

        if (shouldPresent)
        {
            swapChain->Present(0, 0);
            shouldPresent = false;

#if 0
            printf("resources: %lld\n", resources.size());
            printf("textureBindingSets: %lld\n", textureBindingSets.size());
            printf("samplers: %lld\n", samplers.size());
            printf("samplerBindingSets: %lld\n", samplerBindingSets.size());
            printf("framebuffers: %lld\n", framebuffers.size());
            printf("vertexAttributeDescs: %lld\n", vertexAttributeDescs.size());
            printf("inputLayouts: %lld\n", inputLayouts.size());
            printf("pipelines: %lld\n", pipelines.size());
            printf("vertexBuffers: %lld\n", vertexBuffers.size());
            printf("\n");
#endif
        }
    }
}
