#pragma once

#include "ConstantBuffer.h"
#include "MessageReceiver.h"
#include "Device.h"

enum class DirtyFlags
{
    None = 0,
    VsConstants = 1 << 0,
    PsConstants = 1 << 1,
    SharedConstants = 1 << 2,
    Texture = 1 << 3,
    Sampler = 1 << 4,
    FramebufferAndPipeline = 1 << 5,
    InputLayout = 1 << 6,
    VertexBuffer = 1 << 7,
    GraphicsState = 1 << 8,
    All = ~0
};

DEFINE_ENUM_FLAG_OPERATORS(DirtyFlags);

struct Bridge
{
    MessageReceiver msgReceiver;
    Device device;

    DirtyFlags dirtyFlags;

    ComPtr<IDXGISwapChain3> swapChain;
    unsigned int swapChainSurface = 0;
    std::vector<nvrhi::TextureHandle> swapChainTextures;

    std::unordered_map<unsigned int, nvrhi::ResourceHandle> resources;

    nvrhi::CommandListHandle commandList;
    bool openedCommandList = false;

    ConstantBuffer<256> vsConstants {};
    nvrhi::BufferHandle vsConstantBuffer;

    ConstantBuffer<224> psConstants {};
    nvrhi::BufferHandle psConstantBuffer;

    SharedConstantBuffer sharedConstants;
    nvrhi::BufferHandle sharedConstantBuffer;

    nvrhi::BindingLayoutHandle vsBindingLayout;
    nvrhi::BindingSetHandle vsBindingSet;

    nvrhi::BindingLayoutHandle psBindingLayout;
    nvrhi::BindingSetHandle psBindingSet;

    nvrhi::TextureHandle nullTexture;

    nvrhi::BindingLayoutHandle textureBindingLayout;
    nvrhi::BindingSetDesc textureBindingSetDesc;
    std::unordered_map<XXH64_hash_t, nvrhi::BindingSetHandle> textureBindingSets;

    nvrhi::SamplerDesc samplerDescs[16];
    std::unordered_map<XXH64_hash_t, nvrhi::SamplerHandle> samplers;

    nvrhi::BindingLayoutHandle samplerBindingLayout;
    nvrhi::BindingSetDesc samplerBindingSetDesc;
    std::unordered_map<XXH64_hash_t, nvrhi::BindingSetHandle> samplerBindingSets;

    nvrhi::FramebufferDesc framebufferDesc;
    std::unordered_map<XXH64_hash_t, nvrhi::FramebufferHandle> framebuffers;

    std::unordered_map<unsigned int, std::vector<nvrhi::VertexAttributeDesc>> vertexAttributeDescs;
    std::unordered_map<XXH64_hash_t, nvrhi::InputLayoutHandle> inputLayouts;
    unsigned int vertexDeclaration = 0;

    nvrhi::GraphicsPipelineDesc pipelineDesc;
    std::unordered_map<XXH64_hash_t, nvrhi::GraphicsPipelineHandle> pipelines;

    std::unordered_map<unsigned int, nvrhi::BufferHandle> vertexBuffers;
    uint32_t vertexStrides[8] {};
    bool instancing = false;
    uint32_t instanceCount = 0;

    std::unordered_map<XXH64_hash_t, nvrhi::ShaderHandle> shaders;

    nvrhi::GraphicsState graphicsState;

    bool shouldExit = false;
    bool shouldPresent = false;

    Bridge();

    void openCommandList();
    void closeAndExecuteCommandList();

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
    void procMsgExit();
    void procMsgReleaseResource();

    void processMessages();
    void receiveMessages();
};
