#pragma once

#include "CommandQueue.h"
#include "DescriptorHeap.h"
#include "MessageReceiver.h"
#include "PixelShader.h"
#include "SwapChain.h"
#include "Texture.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"
#include "xxHashMap.h"

class Device
{
protected:
    MessageReceiver m_messageReceiver;
    Event m_cpuEvent{ Event::s_cpuEventName };
    Event m_gpuEvent{ Event::s_gpuEventName };

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

    ComPtr<ID3D12Device> m_device;
    ComPtr<D3D12MA::Allocator> m_allocator;

    CommandQueue m_graphicsQueue;
    CommandQueue m_copyQueue;

    DescriptorHeap m_descriptorHeap;
    DescriptorHeap m_samplerDescriptorHeap;
    DescriptorHeap m_rtvDescriptorHeap;
    DescriptorHeap m_dsvDescriptorHeap;

    std::vector<Texture> m_textures;
    std::vector<VertexDeclaration> m_vertexDeclarations;
    std::vector<PixelShader> m_pixelShaders;
    std::vector<VertexShader> m_vertexShaders;
    xxHashMap<uint32_t> m_samplers;
    std::vector<ComPtr<ID3D12Resource>> m_vertexBuffers;
    std::vector<ComPtr<ID3D12Resource>> m_indexBuffers;

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
