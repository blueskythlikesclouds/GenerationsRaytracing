#pragma once

#include "CommandList.h"
#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "Event.h"
#include "MessageReceiver.h"
#include "PixelShader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"
#include "xxHashMap.h"

struct GlobalsVS
{
    float floatConstants[4][256]{};
};

struct GlobalsPS
{
    float floatConstants[4][224]{};
    uint32_t textureIndices[16]{};
    uint32_t samplerIndices[16]{};
};

class Device
{
protected:
    MessageReceiver m_messageReceiver;
    Event m_cpuEvent{ Event::s_cpuEventName };
    Event m_gpuEvent{ Event::s_gpuEventName };

    void createBuffer(
        D3D12_HEAP_TYPE type, 
        uint32_t dataSize, 
        D3D12_RESOURCE_STATES initialState,
        ComPtr<D3D12MA::Allocation>& allocation) const;

    void writeBuffer(
        const uint8_t* memory,
        uint32_t offset,
        uint32_t dataSize,
        ID3D12Resource* dstResource);

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

    SwapChain m_swapChain;
    uint32_t m_swapChainTextureId = 0;
    bool m_shouldPresent = false;

    ComPtr<ID3D12Device> m_device;
    ComPtr<D3D12MA::Allocator> m_allocator;

    CommandQueue m_graphicsQueue;
    CommandQueue m_copyQueue;

    CommandList m_graphicsCommandList;
    CommandList m_copyCommandList;

    DescriptorHeap m_descriptorHeap;
    DescriptorHeap m_samplerDescriptorHeap;
    DescriptorHeap m_rtvDescriptorHeap;
    DescriptorHeap m_dsvDescriptorHeap;

    std::vector<ComPtr<D3D12MA::Allocation>> m_uploadBuffers;
    std::vector<Texture> m_textures;
    std::vector<VertexDeclaration> m_vertexDeclarations;
    std::vector<PixelShader> m_pixelShaders;
    std::vector<VertexShader> m_vertexShaders;
    xxHashMap<uint32_t> m_samplers;
    std::vector<ComPtr<D3D12MA::Allocation>> m_vertexBuffers;
    std::vector<ComPtr<D3D12MA::Allocation>> m_indexBuffers;

    D3D12_CPU_DESCRIPTOR_HANDLE m_renderTargetView{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthStencilView{};

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_pipelineDesc{};
    xxHashMap<ComPtr<ID3D12PipelineState>> m_pipelines;

    std::vector<ComPtr<D3D12MA::Allocation>> m_cbPool;
    uint32_t m_cbIndex = 0;

    GlobalsPS m_globalsPS{};
    GlobalsVS m_globalsVS{};

    D3D12_VIEWPORT m_viewport{};
    RECT m_scissorRect{};

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[16]{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    void flushState();
    void resetState();

    D3D12_GPU_VIRTUAL_ADDRESS makeConstantBuffer(const void* memory);

    void processMessages();

public:
    Device();

    void runLoop();

    void setShouldExit();
    ID3D12Device* getUnderlyingDevice() const;

    CommandQueue& getGraphicsQueue();
    CommandQueue& getCopyQueue();

    DescriptorHeap& getDescriptorHeap();
    DescriptorHeap& getSamplerDescriptorHeap();
    DescriptorHeap& getRtvDescriptorHeap();
    DescriptorHeap& getDsvDescriptorHeap();
};
