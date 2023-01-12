#pragma once

#include "ConstantBuffer.h"
#include "MessageReceiver.h"
#include "Device.h"
#include "RaytracingBridge.h"
#include "VelocityMap.h"

enum class DirtyFlags
{
    None = 0,
    VsConstants = 1 << 0,
    PsConstants = 1 << 1,
    Texture = 1 << 2,
    Sampler = 1 << 3,
    FramebufferAndPipeline = 1 << 4,
    InputLayout = 1 << 5,
    VertexBuffer = 1 << 6,
    GraphicsState = 1 << 7,
    All = ~0
};

DEFINE_ENUM_FLAG_OPERATORS(DirtyFlags);

struct Bridge
{
    std::string directoryPath;

    MessageReceiver msgReceiver;
    Device device;

    DirtyFlags dirtyFlags;

    ComPtr<IDXGISwapChain3> swapChain;
    unsigned int swapChainSurface = 0;
    std::vector<nvrhi::TextureHandle> swapChainTextures;
    uint32_t syncInterval = 0;

    ankerl::unordered_dense::map<unsigned int, nvrhi::ResourceHandle> resources;

    nvrhi::CommandListHandle commandList;
    nvrhi::CommandListHandle commandListForCopy;
    bool openedCommandList = false;

    ConstantBuffer<256> vsConstants {};
    nvrhi::BufferHandle vsConstantBuffer;

    ConstantBuffer<224> psConstants {};
    nvrhi::BufferHandle psConstantBuffer;

    nvrhi::BindingLayoutHandle bindingLayout;
    nvrhi::BindingSetHandle bindingSet;

    nvrhi::TextureHandle nullTexture;

    nvrhi::BindingLayoutHandle textureBindingLayout;
    nvrhi::BindingSetDesc textureBindingSetDesc;
    XXHashMap<nvrhi::BindingSetHandle> textureBindingSetCache;
    XXHashMap<nvrhi::BindingSetHandle> textureBindingSets;

    nvrhi::SamplerDesc samplerDescs[16];
    XXHashMap<nvrhi::SamplerHandle> samplers;

    nvrhi::BindingLayoutHandle samplerBindingLayout;
    nvrhi::BindingSetDesc samplerBindingSetDesc;
    XXHashMap<nvrhi::BindingSetHandle> samplerBindingSetCache;
    XXHashMap<nvrhi::BindingSetHandle> samplerBindingSets;

    nvrhi::FramebufferDesc framebufferDesc;
    XXHashMap<nvrhi::FramebufferHandle> framebuffers;

    nvrhi::ShaderHandle fvfShader;
    bool fvf = false;

    ankerl::unordered_dense::map<unsigned int, std::vector<nvrhi::VertexAttributeDesc>> vertexAttributeDescs;
    XXHashMap<nvrhi::InputLayoutHandle> inputLayouts;
    unsigned int vertexDeclaration = 0;

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    XXHashMap<nvrhi::GraphicsPipelineHandle> pipelines;

    XXHashMap<nvrhi::BufferHandle> vertexBufferCache;
    XXHashMap<nvrhi::BufferHandle> vertexBuffers;

    uint32_t vertexStrides[8] {};
    bool instancing = false;
    uint32_t instanceCount = 0;

    XXHashMap<nvrhi::ShaderHandle> shaders;

    nvrhi::GraphicsState graphicsState;

    std::vector<unsigned int> pendingReleases;

    RaytracingBridge raytracing;
    VelocityMap velocityMap;

    bool shouldExit = false;
    bool shouldPresent = false;

    Bridge(const std::string& directoryPath);

    void openCommandList();
    void openCommandListForCopy();
    void closeAndExecuteCommandLists();

    template<typename T1, typename T2>
    void assignAndUpdateDirtyFlags(T1& dest, const T2& src, DirtyFlags flags)
    {
        if (dest != src)
        {
            dest = src;
            dirtyFlags |= flags;
        }
    }

    void processDirtyFlags();
    void procMsgSetFVF();
    void procMsgInitSwapChain();
    void procMsgPresent();
    void procMsgCreateTexture();
    void procMsgCreateVertexBuffer();
    void procMsgCreateIndexBuffer();
    void procMsgCreateRenderTarget();
    void procMsgCreateDepthStencilSurface();
    void procMsgSetRenderTarget();
    void procMsgSetDepthStencilSurface();
    void procMsgClear();
    void procMsgSetViewport();
    void procMsgSetRenderState();
    void procMsgSetTexture();
    void procMsgSetSamplerState();
    void procMsgSetScissorRect();
    void procMsgDrawPrimitive();
    void procMsgDrawIndexedPrimitive();
    void procMsgDrawPrimitiveUP();
    void procMsgCreateVertexDeclaration();
    void procMsgSetVertexDeclaration();
    void procMsgCreateVertexShader();
    void procMsgSetVertexShader();
    void procMsgSetVertexShaderConstantF();
    void procMsgSetVertexShaderConstantB();
    void procMsgSetStreamSource();
    void procMsgSetStreamSourceFreq();
    void procMsgSetIndices();
    void procMsgCreatePixelShader();
    void procMsgSetPixelShader();
    void procMsgSetPixelShaderConstantF();
    void procMsgSetPixelShaderConstantB();
    void procMsgMakePicture();
    void procMsgWriteBuffer();
    void procMsgWriteTexture();
    void procMsgReleaseResource();

    void processMessages();
    void receiveMessages();
};
