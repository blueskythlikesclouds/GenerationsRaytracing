#include "Device.h"

#include "IndexBuffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "PixelShader.h"
#include "Surface.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"

Device::Device(uint32_t width, uint32_t height)
{
    m_backBuffer.Attach(new Texture(width, height, 1));
}

Texture* Device::getBackBuffer() const
{
    return m_backBuffer.Get();
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::TestCooperativeLevel)

FUNCTION_STUB(UINT, 0, Device::GetAvailableTextureMem)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::EvictManagedResources)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetDirect3D, D3D9** ppD3D9)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetDeviceCaps, D3DCAPS9* pCaps)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetDisplayMode, UINT iSwapChain, D3DDISPLAYMODE* pMode)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetCreationParameters, D3DDEVICE_CREATION_PARAMETERS* pParameters)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetCursorProperties, UINT XHotSpot, UINT YHotSpot, Surface* pCursorBitmap)

FUNCTION_STUB(void, , Device::SetCursorPosition, int X, int Y, DWORD Flags)

FUNCTION_STUB(BOOL, FALSE, Device::ShowCursor, BOOL bShow)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateAdditionalSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetSwapChain, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)

FUNCTION_STUB(UINT, 0, Device::GetNumberOfSwapChains)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::Reset, D3DPRESENT_PARAMETERS* pPresentationParameters)

HRESULT Device::Present(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
{
    s_messageSender.makeSerialMessage<MsgPresent>();
    s_messageSender.sendAllMessages();

    return S_OK;
}

HRESULT Device::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, Surface** ppBackBuffer)
{
    return m_backBuffer->GetSurfaceLevel(0, ppBackBuffer);
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetRasterStatus, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetDialogBoxMode, BOOL bEnableDialogs)

void Device::SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp)
{
    // nothing to do here...
}

FUNCTION_STUB(void, , Device::GetGammaRamp, UINT iSwapChain, D3DGAMMARAMP* pRamp)

HRESULT Device::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, Texture** ppTexture, HANDLE* pSharedHandle)
{
    *ppTexture = new Texture(Width, Height, Levels);

    auto& message = s_messageSender.makeParallelMessage<MsgCreateTexture>();

    message.width = Width;
    message.height = Height;
    message.levels = Levels;
    message.usage = Usage;
    message.format = Format;
    message.textureId = (*ppTexture)->getId();

    s_messageSender.endParallelMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateVolumeTexture, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateCubeTexture, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, CubeTexture** ppCubeTexture, HANDLE* pSharedHandle)

HRESULT Device::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, VertexBuffer** ppVertexBuffer, HANDLE* pSharedHandle)
{
    *ppVertexBuffer = new VertexBuffer(Length);

    auto& message = s_messageSender.makeParallelMessage<MsgCreateVertexBuffer>();

    message.length = Length;
    message.vertexBufferId = (*ppVertexBuffer)->getId();

    s_messageSender.endParallelMessage();

    return S_OK;
}

HRESULT Device::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IndexBuffer** ppIndexBuffer, HANDLE* pSharedHandle)
{
    *ppIndexBuffer = new IndexBuffer(Length);

    auto& message = s_messageSender.makeParallelMessage<MsgCreateIndexBuffer>();

    message.length = Length;
    message.format = Format;
    message.indexBufferId = (*ppIndexBuffer)->getId();

    s_messageSender.endParallelMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateRenderTarget, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, Surface** ppSurface, HANDLE* pSharedHandle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateDepthStencilSurface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, Surface** ppSurface, HANDLE* pSharedHandle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::UpdateSurface, Surface* pSourceSurface, const RECT* pSourceRect, Surface* pDestinationSurface, const POINT* pDestPoint)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::UpdateTexture, BaseTexture* pSourceTexture, BaseTexture* pDestinationTexture)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetRenderTargetData, Surface* pRenderTarget, Surface* pDestSurface)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetFrontBufferData, UINT iSwapChain, Surface* pDestSurface)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::StretchRect, Surface* pSourceSurface, const RECT* pSourceRect, Surface* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::ColorFill, Surface* pSurface, const RECT* pRect, D3DCOLOR color)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateOffscreenPlainSurface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, Surface** ppSurface, HANDLE* pSharedHandle)

HRESULT Device::SetRenderTarget(DWORD RenderTargetIndex, Surface* pRenderTarget)
{
    Texture* texture = pRenderTarget != nullptr ? pRenderTarget->getTexture() : nullptr;
    const uint32_t level = pRenderTarget != nullptr ? pRenderTarget->getLevel() : NULL;

    if (m_renderTargets[RenderTargetIndex].Get() != texture || 
        m_renderTargetLevels[RenderTargetIndex] != level)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetRenderTarget>();

        message.renderTargetIndex = static_cast<uint8_t>(RenderTargetIndex);
        message.textureId = texture != nullptr ? texture->getId() : NULL;
        message.textureLevel = static_cast<uint8_t>(level);

        m_renderTargets[RenderTargetIndex] = texture;
        m_renderTargetLevels[RenderTargetIndex] = level;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetRenderTarget, DWORD RenderTargetIndex, Surface** ppRenderTarget)

HRESULT Device::SetDepthStencilSurface(Surface* pNewZStencil)
{
    Texture* texture = pNewZStencil != nullptr ? pNewZStencil->getTexture() : nullptr;
    const uint32_t level = pNewZStencil != nullptr ? pNewZStencil->getLevel() : NULL;

    if (m_depthStencil.Get() != texture || m_depthStencilLevel != level)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetDepthStencilSurface>();

        message.depthStencilSurfaceId = texture != nullptr ? texture->getId() : NULL;
        message.level = level;

        m_depthStencil = texture;
        m_depthStencilLevel = level;
    }
    return S_OK;
}

HRESULT Device::GetDepthStencilSurface(Surface** ppZStencilSurface)
{
    if (m_depthStencil != nullptr)
        return m_depthStencil->GetSurfaceLevel(m_depthStencilLevel, ppZStencilSurface);

    *ppZStencilSurface = nullptr;
    return S_OK;
}

HRESULT Device::BeginScene()
{
    // nothing to do here...
    return S_OK;
}

HRESULT Device::EndScene()
{
    // nothing to do here...
    return S_OK;
}

HRESULT Device::Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    auto& message = s_messageSender.makeSerialMessage<MsgClear>();

    message.flags = Flags;
    message.color = Color;
    message.z = Z;
    message.stencil = static_cast<uint8_t>(Stencil);

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetTransform, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetTransform, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::MultiplyTransform, D3DTRANSFORMSTATETYPE, const D3DMATRIX*)

HRESULT Device::SetViewport(const D3DVIEWPORT9* pViewport)
{
    if (memcmp(&viewport, pViewport, sizeof(D3DVIEWPORT9)) != 0)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetViewport>();

        message.x = static_cast<uint16_t>(pViewport->X);
        message.y = static_cast<uint16_t>(pViewport->Y);
        message.width = static_cast<uint16_t>(pViewport->Width);
        message.height = static_cast<uint16_t>(pViewport->Height);
        message.minZ = pViewport->MinZ;
        message.maxZ = pViewport->MaxZ;

        viewport = *pViewport;
    }
    return S_OK;
}

HRESULT Device::GetViewport(D3DVIEWPORT9* pViewport)
{
    *pViewport = viewport;
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetMaterial, const D3DMATERIAL9* pMaterial)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetMaterial, D3DMATERIAL9* pMaterial)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetLight, DWORD Index, const D3DLIGHT9*)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetLight, DWORD Index, D3DLIGHT9*)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::LightEnable, DWORD Index, BOOL Enable)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetLightEnable, DWORD Index, BOOL* pEnable)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetClipPlane, DWORD Index, const float* pPlane)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetClipPlane, DWORD Index, float* pPlane)

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    assert(State < 210);

    if (m_renderStates[State] != Value)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetRenderState>();

        message.state = State;
        message.value = Value;

        m_renderStates[State] = Value;
    }

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetRenderState, D3DRENDERSTATETYPE State, DWORD* pValue)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateStateBlock, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::BeginStateBlock)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::EndStateBlock, IDirect3DStateBlock9** ppSB)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetClipStatus, const D3DCLIPSTATUS9* pClipStatus)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetClipStatus, D3DCLIPSTATUS9* pClipStatus)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetTexture, DWORD Stage, BaseTexture** ppTexture)

HRESULT Device::SetTexture(DWORD Stage, BaseTexture* pTexture)
{
    if (m_textures[Stage].Get() != pTexture)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetTexture>();

        message.stage = static_cast<uint8_t>(Stage);
        message.textureId = pTexture != nullptr ? pTexture->getId() : NULL;

        m_textures[Stage] = pTexture;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetTextureStageState, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)

HRESULT Device::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    // nothing to do here...
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetSamplerState, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)

HRESULT Device::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    assert(Type < 14);

    if (m_samplerStates[Sampler][Type] != Value)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetSamplerState>();

        message.sampler = static_cast<uint8_t>(Sampler);
        message.type = static_cast<uint8_t>(Type);
        message.value = Value;

        m_samplerStates[Sampler][Type] = Value;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::ValidateDevice, DWORD* pNumPasses)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetPaletteEntries, UINT PaletteNumber, const PALETTEENTRY* pEntries)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPaletteEntries, UINT PaletteNumber, PALETTEENTRY* pEntries)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetCurrentTexturePalette, UINT PaletteNumber)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetCurrentTexturePalette, UINT* PaletteNumber)

HRESULT Device::SetScissorRect(const RECT* pRect)
{
    if (memcmp(&m_scissorRect, pRect, sizeof(RECT)) != 0)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetScissorRect>();

        message.left = static_cast<uint16_t>(pRect->left);
        message.top = static_cast<uint16_t>(pRect->top);
        message.right = static_cast<uint16_t>(pRect->right);
        message.bottom = static_cast<uint16_t>(pRect->bottom);

        m_scissorRect = *pRect;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetScissorRect, RECT* pRect)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetSoftwareVertexProcessing, BOOL bSoftware)

FUNCTION_STUB(BOOL, FALSE, Device::GetSoftwareVertexProcessing)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetNPatchMode, float nSegments)

FUNCTION_STUB(float, 0.0f, Device::GetNPatchMode)

UINT calculateVertexCount(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
    UINT vertexCount = 0;
    switch (PrimitiveType)
    {
    case D3DPT_POINTLIST:
        vertexCount = PrimitiveCount;
        break;
    case D3DPT_LINELIST:
        vertexCount = PrimitiveCount * 2;
        break;
    case D3DPT_LINESTRIP:
        vertexCount = PrimitiveCount + 1;
        break;
    case D3DPT_TRIANGLELIST:
        vertexCount = PrimitiveCount * 3;
        break;
    case D3DPT_TRIANGLESTRIP:
        vertexCount = PrimitiveCount + 2;
        break;
    case D3DPT_TRIANGLEFAN:
        vertexCount = PrimitiveCount + 2;
        break;
    }
    return vertexCount;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawPrimitive, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    auto& message = s_messageSender.makeSerialMessage<MsgDrawIndexedPrimitive>();

    message.primitiveType = PrimitiveType;
    message.baseVertexIndex = BaseVertexIndex;
    message.startIndex = startIndex;
    message.indexCount = calculateVertexCount(PrimitiveType, primCount);

    return S_OK;
}

HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    const uint32_t vertexCount = calculateVertexCount(PrimitiveType, PrimitiveCount);

    auto& message = s_messageSender.makeSerialMessage<MsgDrawPrimitiveUP>(vertexCount * VertexStreamZeroStride);

    message.primitiveType = PrimitiveType;
    message.vertexCount = vertexCount;
    message.vertexStreamZeroStride = VertexStreamZeroStride;
    memcpy(message.data, pVertexStreamZeroData, vertexCount * VertexStreamZeroStride);

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawIndexedPrimitiveUP, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::ProcessVertices, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, VertexBuffer* pDestBuffer, VertexDeclaration* pVertexDecl, DWORD Flags)

HRESULT Device::CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, VertexDeclaration** ppDecl)
{
    *ppDecl = new VertexDeclaration(pVertexElements);

    auto& message = s_messageSender.makeParallelMessage<MsgCreateVertexDeclaration>(
        (*ppDecl)->getVertexElementsSize() * sizeof(D3DVERTEXELEMENT9));

    message.vertexDeclarationId = (*ppDecl)->getId();
    memcpy(message.data, (*ppDecl)->getVertexElements(), (*ppDecl)->getVertexElementsSize() * sizeof(D3DVERTEXELEMENT9));

    s_messageSender.endParallelMessage();

    return S_OK;
}

HRESULT Device::SetVertexDeclaration(VertexDeclaration* pDecl)
{
    if (m_vertexDeclaration.Get() != pDecl)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetVertexDeclaration>();
        message.vertexDeclarationId = pDecl != nullptr ? pDecl->getId() : NULL;

        m_vertexDeclaration = pDecl;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexDeclaration, VertexDeclaration** ppDecl)

HRESULT Device::SetFVF(DWORD FVF)
{
    auto& vertexDeclaration = m_fvfMap[FVF];
    if (!vertexDeclaration)
        vertexDeclaration.Attach(new VertexDeclaration(FVF));

    return SetVertexDeclaration(vertexDeclaration.Get());
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetFVF, DWORD* pFVF)

HRESULT Device::CreateVertexShader(const DWORD* pFunction, VertexShader** ppShader, DWORD FunctionSize)
{
    *ppShader = new VertexShader();

    auto& message = s_messageSender.makeParallelMessage<MsgCreateVertexShader>(FunctionSize);

    message.vertexShaderId = (*ppShader)->getId();
    memcpy(message.data, pFunction, FunctionSize);

    s_messageSender.endParallelMessage();

    return S_OK;
}

HRESULT Device::SetVertexShader(VertexShader* pShader)
{
    if (m_vertexShader.Get() != pShader)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetVertexShader>();
        message.vertexShaderId = pShader != nullptr ? pShader->getId() : NULL;

        m_vertexShader = pShader;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShader, VertexShader** ppShader)

HRESULT Device::SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount)
{
    auto& message = s_messageSender.makeSerialMessage<MsgSetVertexShaderConstantF>(Vector4fCount * sizeof(float[4]));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, Vector4fCount * sizeof(float[4]));

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetVertexShaderConstantI, UINT StartRegister, const int* pConstantData, UINT Vector4iCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    auto& message = s_messageSender.makeSerialMessage<MsgSetVertexShaderConstantB>(BoolCount * sizeof(BOOL));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, BoolCount * sizeof(BOOL));

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

HRESULT Device::SetStreamSource(UINT StreamNumber, VertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    if (m_streamData[StreamNumber].Get() != pStreamData ||
        m_offsetsInBytes[StreamNumber] != OffsetInBytes ||
        m_strides[StreamNumber] != Stride)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetStreamSource>();

        message.streamNumber = StreamNumber;
        message.streamDataId = pStreamData != nullptr ? pStreamData->getId() : NULL;
        message.offsetInBytes = OffsetInBytes;
        message.stride = Stride;

        m_streamData[StreamNumber] = pStreamData;
        m_offsetsInBytes[StreamNumber] = OffsetInBytes;
        m_strides[StreamNumber] = Stride;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetStreamSource, UINT StreamNumber, VertexBuffer** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)

HRESULT Device::SetStreamSourceFreq(UINT StreamNumber, UINT Setting)
{
    if (m_settings[StreamNumber] != Setting)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetStreamSourceFreq>();

        message.streamNumber = StreamNumber;
        message.setting = Setting;

        m_settings[StreamNumber] = Setting;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetStreamSourceFreq, UINT StreamNumber, UINT* pSetting)

HRESULT Device::SetIndices(IndexBuffer* pIndexData)
{
    if (m_indexData.Get() != pIndexData)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetIndices>();
        message.indexDataId = pIndexData != nullptr ? pIndexData->getId() : NULL;

        m_indexData = pIndexData;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetIndices, IndexBuffer** ppIndexData)

HRESULT Device::CreatePixelShader(const DWORD* pFunction, PixelShader** ppShader, DWORD FunctionSize)
{
    *ppShader = new PixelShader();

    auto& message = s_messageSender.makeParallelMessage<MsgCreatePixelShader>(FunctionSize);

    message.pixelShaderId = (*ppShader)->getId();
    memcpy(message.data, pFunction, FunctionSize);

    s_messageSender.endParallelMessage();

    return S_OK;
}

HRESULT Device::SetPixelShader(PixelShader* pShader)
{
    if (m_pixelShader.Get() != pShader)
    {
        auto& message = s_messageSender.makeSerialMessage<MsgSetPixelShader>();
        message.pixelShaderId = pShader != nullptr ? pShader->getId() : NULL;

        m_pixelShader = pShader;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShader, PixelShader** ppShader)

HRESULT Device::SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount)
{
    auto& message = s_messageSender.makeSerialMessage<MsgSetPixelShaderConstantF>(Vector4fCount * sizeof(float[4]));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, Vector4fCount * sizeof(float[4]));

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetPixelShaderConstantI, UINT StartRegister, const int* pConstantData, UINT Vector4iCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    // nothing to do here...
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawRectPatch, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawTriPatch, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DeletePatch, UINT Handle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateQuery, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)