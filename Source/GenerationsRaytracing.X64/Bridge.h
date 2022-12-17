#pragma once

#include "MessageReceiver.h"
#include "Device.h"

struct Bridge
{
    MessageReceiver msgReceiver;
    Device device;
    ComPtr<IDXGISwapChain2> swapChain;
    bool shouldExit = false;

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
    void procMsgExit();

    void processMessages();
    void receiveMessages();
};
