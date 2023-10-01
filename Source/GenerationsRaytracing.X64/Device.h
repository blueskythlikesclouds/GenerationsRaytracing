#pragma once

#include "Buffer.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Event.h"
#include "MessageReceiver.h"
#include "PixelShader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "UploadBuffer.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"
#include "xxHashMap.h"

struct GlobalsVS
{
    float floatConstants[256][4]{};
};

struct GlobalsPS
{
    float floatConstants[224][4]{};
    uint32_t textureIndices[16]{};
    uint32_t samplerIndices[16]{};
};

enum DirtyFlags
{
    DIRTY_FLAG_NONE = 0,
    DIRTY_FLAG_RENDER_TARGET_AND_DEPTH_STENCIL = 1 << 0,
    DIRTY_FLAG_PIPELINE_DESC = 1 << 1,
    DIRTY_FLAG_GLOBALS_PS = 1 << 2,
    DIRTY_FLAG_GLOBALS_VS = 1 << 3,
    DIRTY_FLAG_VIEWPORT = 1 << 4,
    DIRTY_FLAG_SCISSOR_RECT = 1 << 5,
    DIRTY_FLAG_VERTEX_BUFFER_VIEWS = 1 << 6,
    DIRTY_FLAG_INDEX_BUFFER_VIEW = 1 << 7,
    DIRTY_FLAG_PRIMITIVE_TOPOLOGY = 1 << 8,
    DIRTY_FLAG_SAMPLER_DESC = 1 << 9
};

class Device
{
protected:
    MessageReceiver m_messageReceiver;
    Event m_cpuEvent{ Event::s_cpuEventName };
    Event m_gpuEvent{ Event::s_gpuEventName };

    SwapChain m_swapChain;
    uint32_t m_swapChainTextureId = 0;
    bool m_shouldPresent = false;

    ComPtr<ID3D12Device5> m_device;
    ComPtr<D3D12MA::Allocator> m_allocator;

    CommandQueue m_graphicsQueue;
    CommandQueue m_copyQueue;

    CommandList m_graphicsCommandList;
    CommandList m_copyCommandList;

    DescriptorHeap m_descriptorHeap;
    DescriptorHeap m_samplerDescriptorHeap;
    DescriptorHeap m_rtvDescriptorHeap;
    DescriptorHeap m_dsvDescriptorHeap;

    ComPtr<ID3D12RootSignature> m_rootSignature;

    std::vector<Texture> m_textures;
    std::vector<VertexDeclaration> m_vertexDeclarations;
    std::vector<PixelShader> m_pixelShaders;
    std::vector<VertexShader> m_vertexShaders;
    xxHashMap<uint32_t> m_samplers;
    std::vector<Buffer> m_vertexBuffers;
    std::vector<Buffer> m_indexBuffers;

    std::vector<UploadBuffer> m_uploadBuffers;
    uint32_t m_uploadBufferIndex = 0;
    uint32_t m_uploadBufferOffset = 0;

    std::vector<Texture> m_tempTextures;
    std::vector<ComPtr<D3D12MA::Allocation>> m_tempBuffers;

    D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetView{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilView{};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipelineDesc{};
    xxHashMap<ComPtr<ID3D12PipelineState>> m_pipelines;

    GlobalsPS m_globalsPS{};
    GlobalsVS m_globalsVS{};

    D3D12_SAMPLER_DESC m_samplerDescs[16]{};
    uint32_t m_samplerDescsFirst = 0;
    uint32_t m_samplerDescsLast = _countof(m_samplerDescs) - 1;

    D3D12_VIEWPORT m_viewport{};
    RECT m_scissorRect{};
    bool m_scissorEnable = false;

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[16]{};
    uint32_t m_vertexBufferViewsFirst = 0;
    uint32_t m_vertexBufferViewsLast = _countof(m_vertexBufferViews) - 1;

    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
    D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology{};

    uint32_t m_dirtyFlags = ~0;

    void createBuffer(
        D3D12_HEAP_TYPE type,
        uint32_t dataSize,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_RESOURCE_STATES initialState,
        ComPtr<D3D12MA::Allocation>& allocation) const;

    void writeBuffer(
        const uint8_t* memory,
        uint32_t offset,
        uint32_t dataSize,
        ID3D12Resource* dstResource);

    D3D12_GPU_VIRTUAL_ADDRESS makeBuffer(
        const void* memory,
        uint32_t dataSize,
        uint32_t dataAlignment);

    void setPrimitiveType(D3DPRIMITIVETYPE primitiveType);

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

    void processMessages();
    virtual bool processRaytracingMessage() = 0;

public:
    Device();

    void runLoop();

    void setEvents();
    void setShouldExit();

    ID3D12Device* getUnderlyingDevice() const;

    CommandQueue& getGraphicsQueue();
    CommandQueue& getCopyQueue();

    DescriptorHeap& getDescriptorHeap();
    DescriptorHeap& getSamplerDescriptorHeap();
    DescriptorHeap& getRtvDescriptorHeap();
    DescriptorHeap& getDsvDescriptorHeap();
};
