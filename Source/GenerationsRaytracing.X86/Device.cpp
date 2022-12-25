#include "Device.h"

#include "Buffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "Shader.h"
#include "Surface.h"
#include "Texture.h"
#include "VertexDeclaration.h"

Device::Device(size_t width, size_t height)
{
    swapChainSurface.Attach(new Surface());
    swapChainSurface->texture.Attach(new Texture(width, height));
}

Device::~Device() = default;

FUNCTION_STUB(HRESULT, Device::TestCooperativeLevel)

FUNCTION_STUB(UINT, Device::GetAvailableTextureMem)

FUNCTION_STUB(HRESULT, Device::EvictManagedResources)

FUNCTION_STUB(HRESULT, Device::GetDirect3D, D3D9** ppD3D9)

FUNCTION_STUB(HRESULT, Device::GetDeviceCaps, D3DCAPS9* pCaps)

FUNCTION_STUB(HRESULT, Device::GetDisplayMode, UINT iSwapChain, D3DDISPLAYMODE* pMode)

FUNCTION_STUB(HRESULT, Device::GetCreationParameters, D3DDEVICE_CREATION_PARAMETERS *pParameters)

FUNCTION_STUB(HRESULT, Device::SetCursorProperties, UINT XHotSpot, UINT YHotSpot, Surface* pCursorBitmap)

FUNCTION_STUB(void, Device::SetCursorPosition, int X, int Y, DWORD Flags)

FUNCTION_STUB(BOOL, Device::ShowCursor, BOOL bShow)

FUNCTION_STUB(HRESULT, Device::CreateAdditionalSwapChain, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain)

FUNCTION_STUB(HRESULT, Device::GetSwapChain, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain)

FUNCTION_STUB(UINT, Device::GetNumberOfSwapChains)

FUNCTION_STUB(HRESULT, Device::Reset, D3DPRESENT_PARAMETERS* pPresentationParameters)

HRESULT Device::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
    msgSender.start<MsgPresent>();
    msgSender.finish();

    msgSender.commitAllMessages();

    return S_OK;
}

HRESULT Device::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, Surface** ppBackBuffer)
{
    swapChainSurface.CopyTo(ppBackBuffer);
    return S_OK;
}
    
FUNCTION_STUB(HRESULT, Device::GetRasterStatus, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus)

FUNCTION_STUB(HRESULT, Device::SetDialogBoxMode, BOOL bEnableDialogs)

FUNCTION_STUB(void, Device::SetGammaRamp, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp)

FUNCTION_STUB(void, Device::GetGammaRamp, UINT iSwapChain, D3DGAMMARAMP* pRamp)

HRESULT Device::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, Texture** ppTexture, HANDLE* pSharedHandle)
{
    *ppTexture = new Texture(Width, Height);

    assert((Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) ||
        Format == D3DFMT_A8B8G8R8 ||
        Format == D3DFMT_A8R8G8B8 ||
        Format == D3DFMT_X8B8G8R8 ||
        Format == D3DFMT_X8R8G8B8);

    const auto msg = msgSender.start<MsgCreateTexture>();

    msg->width = Width;
    msg->height = Height;
    msg->levels = Levels;
    msg->usage = Usage;
    msg->format = Format;
    msg->texture = (*ppTexture)->id;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::CreateVolumeTexture, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)

FUNCTION_STUB(HRESULT, Device::CreateCubeTexture, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, Texture** ppCubeTexture, HANDLE* pSharedHandle)

HRESULT Device::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, Buffer** ppVertexBuffer, HANDLE* pSharedHandle)
{
    *ppVertexBuffer = new Buffer(Length);

    const auto msg = msgSender.start<MsgCreateVertexBuffer>();

    msg->length = Length;
    msg->vertexBuffer = (*ppVertexBuffer)->id;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, Buffer** ppIndexBuffer, HANDLE* pSharedHandle)
{
    *ppIndexBuffer = new Buffer(Length);

    const auto msg = msgSender.start<MsgCreateIndexBuffer>();

    msg->length = Length;
    msg->format = Format;
    msg->indexBuffer = (*ppIndexBuffer)->id;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, Surface** ppSurface, HANDLE* pSharedHandle)
{
    *ppSurface = new Surface();
    (*ppSurface)->texture.Attach(new Texture(Width, Height));

    const auto msg = msgSender.start<MsgCreateRenderTarget>();

    msg->width = Width;
    msg->height = Height;
    msg->format = Format;
    msg->surface = (*ppSurface)->texture->id;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, Surface** ppSurface, HANDLE* pSharedHandle)
{
    *ppSurface = new Surface();
    (*ppSurface)->texture.Attach(new Texture(Width, Height));

    const auto msg = msgSender.start<MsgCreateDepthStencilSurface>();

    msg->width = Width;
    msg->height = Height;
    msg->format = Format;
    msg->surface = (*ppSurface)->texture->id;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::UpdateSurface, Surface* pSourceSurface, CONST RECT* pSourceRect, Surface* pDestinationSurface, CONST POINT* pDestPoint)

FUNCTION_STUB(HRESULT, Device::UpdateTexture, BaseTexture* pSourceTexture, BaseTexture* pDestinationTexture)

FUNCTION_STUB(HRESULT, Device::GetRenderTargetData, Surface* pRenderTarget, Surface* pDestSurface)

FUNCTION_STUB(HRESULT, Device::GetFrontBufferData, UINT iSwapChain, Surface* pDestSurface)

HRESULT Device::StretchRect(Surface* pSourceSurface, CONST RECT* pSourceRect, Surface* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::ColorFill, Surface* pSurface, CONST RECT* pRect, D3DCOLOR color)

HRESULT Device::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, Surface** ppSurface, HANDLE* pSharedHandle)
{
    *ppSurface = nullptr;
    return E_FAIL;
}

HRESULT Device::SetRenderTarget(DWORD RenderTargetIndex, Surface* pRenderTarget)
{
    if (renderTargets[RenderTargetIndex].Get() == pRenderTarget) return S_OK;
    renderTargets[RenderTargetIndex] = pRenderTarget;

    const auto msg = msgSender.start<MsgSetRenderTarget>();

    msg->index = RenderTargetIndex;
    msg->surface = pRenderTarget ? pRenderTarget->texture->id : NULL;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::GetRenderTarget(DWORD RenderTargetIndex, Surface** ppRenderTarget)
{
    return S_OK;
}

HRESULT Device::SetDepthStencilSurface(Surface* pNewZStencil)
{
    if (depthStencilSurface.Get() == pNewZStencil) return S_OK;
    depthStencilSurface = pNewZStencil;

    const auto msg = msgSender.start<MsgSetDepthStencilSurface>();

    msg->surface = pNewZStencil ? pNewZStencil->texture->id : NULL;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::GetDepthStencilSurface(Surface** ppZStencilSurface)
{
    depthStencilSurface.CopyTo(ppZStencilSurface);
    return S_OK;
}

HRESULT Device::BeginScene()
{
    return S_OK;
}

HRESULT Device::EndScene()
{
    return S_OK;
}

HRESULT Device::Clear(DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    const auto msg = msgSender.start<MsgClear>();

    msg->flags = Flags;
    msg->color = Color;
    msg->z = Z;
    msg->stencil = Stencil;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::SetTransform, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, Device::GetTransform, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, Device::MultiplyTransform, D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*)

HRESULT Device::SetViewport(CONST D3DVIEWPORT9* pViewport)
{
    if (memcmp(&viewport, pViewport, sizeof(D3DVIEWPORT9)) == 0) return S_OK;
    viewport = *pViewport;

    const auto msg = msgSender.start<MsgSetViewport>();

    msg->x = pViewport->X;
    msg->y = pViewport->Y;
    msg->width = pViewport->Width;
    msg->height = pViewport->Height;
    msg->minZ = pViewport->MinZ;
    msg->maxZ = pViewport->MaxZ;

    msgSender.finish();

    return S_OK;
}

HRESULT Device::GetViewport(D3DVIEWPORT9* pViewport)
{
    *pViewport = viewport;
    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::SetMaterial, CONST D3DMATERIAL9* pMaterial)

FUNCTION_STUB(HRESULT, Device::GetMaterial, D3DMATERIAL9* pMaterial)

FUNCTION_STUB(HRESULT, Device::SetLight, DWORD Index, CONST D3DLIGHT9*)

FUNCTION_STUB(HRESULT, Device::GetLight, DWORD Index, D3DLIGHT9*)

FUNCTION_STUB(HRESULT, Device::LightEnable, DWORD Index, BOOL Enable)

FUNCTION_STUB(HRESULT, Device::GetLightEnable, DWORD Index, BOOL* pEnable)

FUNCTION_STUB(HRESULT, Device::SetClipPlane, DWORD Index, CONST float* pPlane)

FUNCTION_STUB(HRESULT, Device::GetClipPlane, DWORD Index, float* pPlane)

HRESULT Device::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
    if (renderStates[State] == Value) return S_OK;
    renderStates[State] = Value;

    const auto msg = msgSender.start<MsgSetRenderState>();

    msg->state = State;
    msg->value = Value;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetRenderState, D3DRENDERSTATETYPE State, DWORD* pValue)

FUNCTION_STUB(HRESULT, Device::CreateStateBlock, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)

FUNCTION_STUB(HRESULT, Device::BeginStateBlock)

FUNCTION_STUB(HRESULT, Device::EndStateBlock, IDirect3DStateBlock9** ppSB)

FUNCTION_STUB(HRESULT, Device::SetClipStatus, CONST D3DCLIPSTATUS9* pClipStatus)

FUNCTION_STUB(HRESULT, Device::GetClipStatus, D3DCLIPSTATUS9* pClipStatus)

FUNCTION_STUB(HRESULT, Device::GetTexture, DWORD Stage, BaseTexture** ppTexture)

HRESULT Device::SetTexture(DWORD Stage, BaseTexture* pTexture)
{
    if (textures[Stage].Get() == pTexture) return S_OK;
    textures[Stage] = pTexture;

    const auto msg = msgSender.start<MsgSetTexture>();

    msg->stage = Stage;
    msg->texture = pTexture ? pTexture->id : NULL;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetTextureStageState, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)

HRESULT Device::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetSamplerState, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)

HRESULT Device::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    if (samplerStates[Sampler][Type] == Value) return S_OK;
    samplerStates[Sampler][Type] = Value;

    const auto msg = msgSender.start<MsgSetSamplerState>();

    msg->sampler = Sampler;
    msg->type = Type;
    msg->value = Value;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::ValidateDevice, DWORD* pNumPasses)

FUNCTION_STUB(HRESULT, Device::SetPaletteEntries, UINT PaletteNumber, CONST PALETTEENTRY* pEntries)

FUNCTION_STUB(HRESULT, Device::GetPaletteEntries, UINT PaletteNumber, PALETTEENTRY* pEntries)

FUNCTION_STUB(HRESULT, Device::SetCurrentTexturePalette, UINT PaletteNumber)

FUNCTION_STUB(HRESULT, Device::GetCurrentTexturePalette, UINT *PaletteNumber)

HRESULT Device::SetScissorRect(CONST RECT* pRect)
{
    if (memcmp(&scissorRect, pRect, sizeof(RECT)) == 0) return S_OK;
    scissorRect = *pRect;

    const auto msg = msgSender.start<MsgSetScissorRect>();

    msg->left = pRect->left;
    msg->top = pRect->top;
    msg->right = pRect->right;
    msg->bottom = pRect->bottom;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetScissorRect, RECT* pRect)

FUNCTION_STUB(HRESULT, Device::SetSoftwareVertexProcessing, BOOL bSoftware)

FUNCTION_STUB(BOOL, Device::GetSoftwareVertexProcessing)

FUNCTION_STUB(HRESULT, Device::SetNPatchMode, float nSegments)

FUNCTION_STUB(float, Device::GetNPatchMode)

static UINT calculateIndexCount(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
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

HRESULT Device::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    const auto msg = msgSender.start<MsgDrawPrimitive>();

    msg->primitiveType = PrimitiveType;
    msg->startVertex = StartVertex;
    msg->primitiveCount = calculateIndexCount(PrimitiveType, PrimitiveCount);

    msgSender.finish();

    return S_OK;
}       
        
HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    const auto msg = msgSender.start<MsgDrawIndexedPrimitive>();

    msg->primitiveType = PrimitiveType;
    msg->baseVertexIndex = BaseVertexIndex;
    msg->startIndex = startIndex;
    msg->primitiveCount = calculateIndexCount(PrimitiveType, primCount);

    msgSender.finish();

    return S_OK;
}
        
HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    const unsigned int primitiveCount = calculateIndexCount(PrimitiveType, PrimitiveCount);
    const unsigned int vertexStreamZeroSize = primitiveCount * VertexStreamZeroStride;

    const auto msg = msgSender.start<MsgDrawPrimitiveUP>(vertexStreamZeroSize);

    msg->primitiveType = PrimitiveType;
    msg->primitiveCount = primitiveCount;
    msg->vertexStreamZeroSize = vertexStreamZeroSize;
    msg->vertexStreamZeroStride = VertexStreamZeroStride;

    memcpy(MSG_DATA_PTR(msg), pVertexStreamZeroData, vertexStreamZeroSize);

    msgSender.finish();

    return S_OK;
}       
        
FUNCTION_STUB(HRESULT, Device::DrawIndexedPrimitiveUP, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)

FUNCTION_STUB(HRESULT, Device::ProcessVertices, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, Buffer* pDestBuffer, VertexDeclaration* pVertexDecl, DWORD Flags)

HRESULT Device::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements, VertexDeclaration** ppDecl)
{
    *ppDecl = new VertexDeclaration(pVertexElements);

    const auto msg = msgSender.start<MsgCreateVertexDeclaration>((*ppDecl)->getVertexElementsSize());

    msg->vertexElementCount = (*ppDecl)->vertexElements.size();
    msg->vertexDeclaration = (*ppDecl)->id;

    memcpy(MSG_DATA_PTR(msg), (*ppDecl)->vertexElements.data(), (*ppDecl)->getVertexElementsSize());

    msgSender.finish();

    return S_OK;
}

HRESULT Device::SetVertexDeclaration(VertexDeclaration* pDecl)
{
    if (vertexDeclaration.Get() == pDecl) return S_OK;

    vertexDeclaration = pDecl;
    fvf = 0;

    const auto msg = msgSender.start<MsgSetVertexDeclaration>();

    msg->vertexDeclaration = pDecl ? pDecl->id : NULL;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetVertexDeclaration, VertexDeclaration** ppDecl)

HRESULT Device::SetFVF(DWORD FVF)
{
    if (fvf == FVF) return S_OK;

    vertexDeclaration = nullptr;
    fvf = FVF;

    const auto msg = msgSender.start<MsgSetFVF>();

    msg->fvf = FVF;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetFVF, DWORD* pFVF)

HRESULT Device::CreateVertexShader(CONST DWORD* pFunction, Shader** ppShader, DWORD FunctionSize)
{
    *ppShader = new Shader();

    const auto msg = msgSender.start<MsgCreateVertexShader>(FunctionSize);

    msg->shader = (*ppShader)->id;
    msg->functionSize = FunctionSize;

    memcpy(MSG_DATA_PTR(msg), pFunction, FunctionSize);

    msgSender.finish();

    return S_OK;
}

HRESULT Device::SetVertexShader(Shader* pShader)
{
    if (vertexShader.Get() == pShader) return S_OK;
    vertexShader = pShader;

    const auto msg = msgSender.start<MsgSetVertexShader>();

    msg->shader = pShader ? pShader->id : NULL;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetVertexShader, Shader** ppShader)

HRESULT Device::SetVertexShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    const auto msg = msgSender.start<MsgSetVertexShaderConstantF>(Vector4fCount * sizeof(FLOAT[4]));

    msg->startRegister = StartRegister;
    msg->vector4fCount = Vector4fCount;

    memcpy(MSG_DATA_PTR(msg), pConstantData, Vector4fCount * sizeof(FLOAT[4]));

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetVertexShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

HRESULT Device::SetVertexShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetVertexShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetVertexShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    void* dest = &boolConstantsVS[StartRegister];
    const size_t destSize = BoolCount * sizeof(BOOL);

    if (memcmp(dest, pConstantData, destSize) == 0) return S_OK;
    memcpy(dest, pConstantData, destSize);

    const auto msg = msgSender.start<MsgSetVertexShaderConstantB>(destSize);

    msg->startRegister = StartRegister;
    msg->boolCount = BoolCount;

    memcpy(MSG_DATA_PTR(msg), pConstantData, destSize);

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetVertexShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

HRESULT Device::SetStreamSource(UINT StreamNumber, Buffer* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    if (streamBuffers[StreamNumber].Get() == pStreamData &&
        streamOffsets[StreamNumber] == OffsetInBytes &&
        streamStrides[StreamNumber] == Stride)
        return S_OK;

    streamBuffers[StreamNumber] = pStreamData;
    streamOffsets[StreamNumber] = OffsetInBytes;
    streamOffsets[StreamNumber] = Stride;

    const auto msg = msgSender.start<MsgSetStreamSource>();

    msg->streamNumber = StreamNumber;
    msg->streamData = pStreamData ? pStreamData->id : NULL;
    msg->offsetInBytes = OffsetInBytes;
    msg->stride = Stride;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetStreamSource, UINT StreamNumber, Buffer** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)

HRESULT Device::SetStreamSourceFreq(UINT StreamNumber, UINT Setting)
{
    if (streamSettings[StreamNumber] == Setting) return S_OK;
    streamSettings[StreamNumber] = Setting;

    const auto msg = msgSender.start<MsgSetStreamSourceFreq>();

    msg->streamNumber = StreamNumber;
    msg->setting = Setting;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetStreamSourceFreq, UINT StreamNumber, UINT* pSetting)

HRESULT Device::SetIndices(Buffer* pIndexData)
{
    if (indices.Get() == pIndexData) return S_OK;
    indices = pIndexData;

    const auto msg = msgSender.start<MsgSetIndices>();

    msg->indexData = pIndexData ? pIndexData->id : NULL;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetIndices, Buffer** ppIndexData)

HRESULT Device::CreatePixelShader(CONST DWORD* pFunction, Shader** ppShader, DWORD FunctionSize)
{
    *ppShader = new Shader();

    const auto msg = msgSender.start<MsgCreatePixelShader>(FunctionSize);

    msg->shader = (*ppShader)->id;
    msg->functionSize = FunctionSize;

    memcpy(MSG_DATA_PTR(msg), pFunction, FunctionSize);

    msgSender.finish();

    return S_OK;
}

HRESULT Device::SetPixelShader(Shader* pShader)
{
    if (pixelShader.Get() == pShader) return S_OK;
    pixelShader = pShader;

    const auto msg = msgSender.start<MsgSetPixelShader>();

    msg->shader = pShader ? pShader->id : NULL;

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetPixelShader, Shader** ppShader)

HRESULT Device::SetPixelShaderConstantF(UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    const auto msg = msgSender.start<MsgSetPixelShaderConstantF>(Vector4fCount * sizeof(FLOAT[4]));

    msg->startRegister = StartRegister;
    msg->vector4fCount = Vector4fCount;

    memcpy(MSG_DATA_PTR(msg), pConstantData, Vector4fCount * sizeof(FLOAT[4]));

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetPixelShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

HRESULT Device::SetPixelShaderConstantI(UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetPixelShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetPixelShaderConstantB(UINT StartRegister, CONST BOOL* pConstantData, UINT  BoolCount)
{
    void* dest = &boolConstantsPS[StartRegister];
    const size_t destSize = BoolCount * sizeof(BOOL);

    if (memcmp(dest, pConstantData, destSize) == 0) return S_OK;
    memcpy(dest, pConstantData, destSize);

    const auto msg = msgSender.start<MsgSetPixelShaderConstantB>(destSize);

    msg->startRegister = StartRegister;
    msg->boolCount = BoolCount;

    memcpy(MSG_DATA_PTR(msg), pConstantData, destSize);

    msgSender.finish();

    return S_OK;
}

FUNCTION_STUB(HRESULT, Device::GetPixelShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

FUNCTION_STUB(HRESULT, Device::DrawRectPatch, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)

FUNCTION_STUB(HRESULT, Device::DrawTriPatch, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)

FUNCTION_STUB(HRESULT, Device::DeletePatch, UINT Handle)

FUNCTION_STUB(HRESULT, Device::CreateQuery, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)