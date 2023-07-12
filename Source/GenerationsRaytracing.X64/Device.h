#pragma once

#include "MessageReceiver.h"
#include "SwapChain.h"

class Device
{
protected:
    MessageReceiver m_messageReceiver;

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
    ComPtr<ID3D12Device> m_device;

public:
    Device();

    void receiveMessages();

    ID3D12Device* getUnderlyingDevice() const;
};
