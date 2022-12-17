#include "Bridge.h"

#include "Message.h"

void Bridge::procMsgSetFVF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetFVF>();
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
}

void Bridge::procMsgPresent()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgPresent>();
    swapChain->Present(0, 0);
}

void Bridge::procMsgCreateTexture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateTexture>();
}

void Bridge::procMsgCreateVertexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexBuffer>();
}

void Bridge::procMsgCreateIndexBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateIndexBuffer>();
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
}

void Bridge::procMsgSetDepthStencilSurface()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetDepthStencilSurface>();
}

void Bridge::procMsgClear()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgClear>();
}

void Bridge::procMsgSetViewport()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetViewport>();
}

void Bridge::procMsgSetRenderState()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetRenderState>();
}

void Bridge::procMsgSetTexture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetTexture>();
}

void Bridge::procMsgSetSamplerState()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetSamplerState>();
}

void Bridge::procMsgSetScissorRect()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetScissorRect>();
}

void Bridge::procMsgDrawPrimitive()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawPrimitive>();
}

void Bridge::procMsgDrawIndexedPrimitive()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawIndexedPrimitive>();
}

void Bridge::procMsgDrawPrimitiveUP()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgDrawPrimitiveUP>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vertexStreamZeroSize);
}

void Bridge::procMsgCreateVertexDeclaration()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexDeclaration>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vertexElementCount * sizeof(D3DVERTEXELEMENT9));
}

void Bridge::procMsgSetVertexDeclaration()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexDeclaration>();
}

void Bridge::procMsgCreateVertexShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreateVertexShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);
}

void Bridge::procMsgSetVertexShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShader>();
}

void Bridge::procMsgSetVertexShaderConstantF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShaderConstantF>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vector4fCount * sizeof(FLOAT[4]));
}

void Bridge::procMsgSetVertexShaderConstantB()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetVertexShaderConstantB>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->boolCount * sizeof(BOOL));
}

void Bridge::procMsgSetStreamSource()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetStreamSource>();
}

void Bridge::procMsgSetStreamSourceFreq()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetStreamSourceFreq>();
}

void Bridge::procMsgSetIndices()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetIndices>();
}

void Bridge::procMsgCreatePixelShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgCreatePixelShader>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->functionSize);
}

void Bridge::procMsgSetPixelShader()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShader>();
}

void Bridge::procMsgSetPixelShaderConstantF()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShaderConstantF>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->vector4fCount * sizeof(FLOAT[4]));
}

void Bridge::procMsgSetPixelShaderConstantB()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgSetPixelShaderConstantB>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->boolCount * sizeof(BOOL));
}

void Bridge::procMsgMakePicture()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgMakePicture>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);
}

void Bridge::procMsgWriteBuffer()
{
    const auto msg = msgReceiver.getMsgAndMoveNext<MsgWriteBuffer>();
    const void* data = msgReceiver.getDataAndMoveNext(msg->size);
}

void Bridge::procMsgExit()
{
    shouldExit = true;
}

void Bridge::processMessages()
{
    while (msgReceiver.hasNext())
    {
        switch (msgReceiver.getMsgId())
        {
        case MsgSetFVF::ID:                    procMsgSetFVF(); break;
        case MsgInitSwapChain::ID:           procMsgInitSwapChain(); break;
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
        default:                               assert(0 && "Unknown message type"); break;
        }
    }
}

void Bridge::receiveMessages()
{
    while (!shouldExit)
    {
        processMessages();
    }
}
