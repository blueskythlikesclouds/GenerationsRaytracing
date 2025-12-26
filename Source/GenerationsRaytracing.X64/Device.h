#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Event.h"
#include "MessageReceiver.h"
#include "PixelShader.h"
#include "ShaderCache.h"
#include "SwapChain.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"
#include "xxHashMap.h"

struct alignas(0x10) GlobalsVS
{
    float floatConstants[256][4]{};
};

static_assert(sizeof(GlobalsVS) == 0x1000);

struct alignas(0x10) GlobalsPS
{
    float floatConstants[224][4]{};
    uint32_t textureIndices[16]{};
    uint32_t samplerIndices[16]{};
    uint32_t enableAlphaTest{};
    float alphaThreshold{};
    uint32_t booleans{};
    uint32_t exposureTextureId{};
};

static_assert(sizeof(GlobalsPS) <= 0x1000);

enum DirtyFlags
{
    DIRTY_FLAG_NONE = 0,
    DIRTY_FLAG_ROOT_SIGNATURE = 1 << 0,
    DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL = 1 << 1,
    DIRTY_FLAG_PIPELINE_DESC = 1 << 2,
    DIRTY_FLAG_GLOBALS_PS = 1 << 3,
    DIRTY_FLAG_GLOBALS_VS = 1 << 4,
    DIRTY_FLAG_VIEWPORT = 1 << 5,
    DIRTY_FLAG_SCISSOR_RECT = 1 << 6,
    DIRTY_FLAG_VERTEX_DECLARATION = 1 << 7,
    DIRTY_FLAG_VERTEX_BUFFER_VIEWS = 1 << 8,
    DIRTY_FLAG_INDEX_BUFFER_VIEW = 1 << 9,
    DIRTY_FLAG_PRIMITIVE_TOPOLOGY = 1 << 10,
    DIRTY_FLAG_SAMPLER_DESC = 1 << 11
};

static constexpr size_t NUM_FRAMES = 2;

class Device
{
protected:
    MessageReceiver m_messageReceiver;
    Event m_swapChainEvent{ Event::s_swapChainEventName };

    uint32_t m_frame = 0;
    uint32_t m_nextFrame = 1;
    bool m_shouldPresent = false;

    ComPtr<ID3D12Device5> m_device;
    ComPtr<D3D12MA::Allocator> m_allocator;

    bool m_gpuUploadHeapSupported = false;

    CommandQueue m_graphicsQueue;
    CommandQueue m_copyQueue;

    CommandList m_graphicsCommandLists[NUM_FRAMES];
    CommandList m_copyCommandLists[NUM_FRAMES];

    uint64_t m_fenceValue = 1;
    uint64_t m_fenceValues[NUM_FRAMES]{};

    DescriptorHeap m_descriptorHeap;
    DescriptorHeap m_samplerDescriptorHeap;
    DescriptorHeap m_rtvDescriptorHeap;
    DescriptorHeap m_dsvDescriptorHeap;

    ComPtr<ID3D12RootSignature> m_rootSignature;

    std::vector<Texture> m_textures;
    std::vector<VertexDeclaration> m_vertexDeclarations;
    std::vector<PixelShader> m_pixelShaders;
    std::vector<VertexShader> m_vertexShaders;
    std::unique_ptr<ShaderCache> m_shaderCache{ new ShaderCache() };
    xxHashMap<uint32_t> m_samplers;
    std::vector<Buffer> m_vertexBuffers;
    std::vector<Buffer> m_indexBuffers;

    std::vector<UploadBuffer> m_uploadBuffers[NUM_FRAMES];
    uint32_t m_uploadBufferIndex = 0;
    uint32_t m_uploadBufferOffset = 0;

    std::vector<Texture> m_tempTextures[NUM_FRAMES];
    std::vector<ComPtr<D3D12MA::Allocation>> m_tempBuffers[NUM_FRAMES];
    std::vector<uint32_t> m_tempDescriptorIds[NUM_FRAMES];

    D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetViews[4]{};
    ID3D12Resource* m_renderTargetTextures[4]{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilView{};
    ID3D12Resource* m_depthStencilTexture = nullptr;

    xxHashMap<ComPtr<ID3D12PipelineState>> m_pipelines;
    ID3D12PipelineState* m_curPipeline = nullptr;

    uint32_t m_samplerDescsFirst = 0;
    uint32_t m_samplerDescsLast = _countof(m_samplerDescs) - 1;

    D3D12_VIEWPORT m_viewport{};
    RECT m_scissorRect{};
    uint32_t m_vertexDeclarationId = 0;
    uint32_t m_pixelShaderId = 0;
    uint32_t m_vertexShaderId = 0;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[16]{};
    uint32_t m_vertexBufferViewsFirst = 0;
    uint32_t m_vertexBufferViewsLast = _countof(m_vertexBufferViews) - 1;

    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology{};

    bool m_instancing = false;
    uint32_t m_instanceCount = 1;

    uint32_t m_dirtyFlags = ~0;

    uint32_t m_anisotropicFiltering = 0;

    SwapChain m_swapChain;
    uint32_t m_swapChainTextureId = 0;

    ComPtr<ID3D12RootSignature> m_copyHdrTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyHdrTexturePipeline;

    // Place chonky variables at the end.
    GlobalsVS m_globalsVS{};
    GlobalsPS m_globalsPS{};
    D3D12_SAMPLER_DESC m_samplerDescs[16]{};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipelineDesc{};

    void createBuffer(
        D3D12_HEAP_TYPE type,
        uint32_t dataSize,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initialState,
        ComPtr<D3D12MA::Allocation>& allocation) const;

    void writeBuffer(
        const void* memory,
        uint32_t offset,
        uint32_t dataSize,
        ID3D12Resource* dstResource,
        bool mapWrite);

    D3D12_GPU_VIRTUAL_ADDRESS createBuffer(
        const void* memory,
        uint32_t dataSize,
        uint32_t dataAlignment);

    void setPrimitiveType(D3DPRIMITIVETYPE primitiveType);

    void setDescriptorHeaps();
    void flushGraphicsState();

    void procMsgPadding();
    void procMsgCreateSwapChain();
    void procMsgSetRenderTarget();
    void procMsgCreateVertexDeclaration();
    void procMsgCreatePixelShader();
    void procMsgCreateVertexShader();
    void procMsgSetRenderState();
    void procMsgCreateTexture();
    void procMsgSetTexture();
    void procMsgSetDepthStencilSurface();
    void procMsgClear();
    void procMsgSetVertexShader();
    void procMsgSetPixelShader();
    void procMsgSetPixelShaderConstantF();
    void procMsgSetVertexShaderConstantF();
    void procMsgSetVertexShaderConstantB();
    void procMsgSetSamplerState();
    void procMsgSetViewport();
    void procMsgSetScissorRect();
    void procMsgSetVertexDeclaration();
    void procMsgDrawPrimitiveUP();
    void procMsgSetStreamSource();
    void procMsgSetIndices();
    void procMsgPresent();
    void procMsgProcessWindowMessages();
    void procMsgWaitOnSwapChain();
    void procMsgCreateVertexBuffer();
    void procMsgWriteVertexBuffer();
    void procMsgCreateIndexBuffer();
    void procMsgWriteIndexBuffer();
    void procMsgWriteTexture();
    void procMsgMakeTexture();
    void procMsgDrawIndexedPrimitive();
    void procMsgSetStreamSourceFreq();
    void procMsgReleaseResource();
    void procMsgDrawPrimitive();
    void procMsgCopyVertexBuffer();
    void procMsgSetPixelShaderConstantB();
    void procMsgSaveShaderCache();
    void procMsgDrawIndexedPrimitiveUP();
    void procMsgShowCursor();
    void procMsgCopyHdrTexture();

    void beginCommandList();

    virtual bool processRaytracingMessage() { return false; }
    virtual void releaseRaytracingResources() {}

public:
    Device(const IniFile& iniFile);
    virtual ~Device();

    void runLoop();

    ID3D12Device* getUnderlyingDevice() const;
    D3D12MA::Allocator* getAllocator() const;

    CommandQueue& getGraphicsQueue();
    const CommandQueue& getGraphicsQueue() const;
    CommandQueue& getCopyQueue();

    CommandList& getGraphicsCommandList();
    ID3D12GraphicsCommandList4* getUnderlyingGraphicsCommandList() const;

    CommandList& getCopyCommandList();
    ID3D12GraphicsCommandList4* getUnderlyingCopyCommandList() const;

    template<typename T>
    void executeCopyCommandList(const T& function);

    DescriptorHeap& getDescriptorHeap();
    DescriptorHeap& getSamplerDescriptorHeap();
    DescriptorHeap& getRtvDescriptorHeap();
    const DescriptorHeap& getRtvDescriptorHeap() const;
    DescriptorHeap& getDsvDescriptorHeap();

    const GlobalsVS& getGlobalsVS() const;
    const GlobalsPS& getGlobalsPS() const;

    SwapChain& getSwapChain();
};

#include "Device.inl"
