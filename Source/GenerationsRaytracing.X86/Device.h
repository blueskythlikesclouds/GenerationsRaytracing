#pragma once

#include "Unknown.h"

class BaseTexture;
class CubeTexture;
class D3D9;
class IndexBuffer;
class PixelShader;
class Surface;
class Texture;
class VertexBuffer;
class VertexDeclaration;
class VertexShader;

class Device : public Unknown
{
protected:
    ComPtr<Texture> m_backBuffer;

    ComPtr<Texture> m_renderTargets[4];
    uint32_t m_renderTargetLevels[4]{};

    D3DVIEWPORT9 viewport{};

    DWORD m_renderStates[210]{};

    ComPtr<BaseTexture> m_textures[16];

    ComPtr<Texture> m_depthStencil;
    uint32_t m_depthStencilLevel = 0;

    ComPtr<VertexShader> m_vertexShader;
    ComPtr<PixelShader> m_pixelShader;

    DWORD m_samplerStates[16][14];

    RECT m_scissorRect{};

    ComPtr<VertexDeclaration> m_vertexDeclaration;

    std::unordered_map<DWORD, ComPtr<VertexDeclaration>> m_fvfMap;

    ComPtr<VertexBuffer> m_streamData[16];
    UINT m_offsetsInBytes[16]{};
    UINT m_strides[16]{};

    ComPtr<IndexBuffer> m_indexData;

    UINT m_settings[16]{};

public:
    Device(uint32_t width, uint32_t height);

    Texture* getBackBuffer() const;

    virtual HRESULT TestCooperativeLevel() final;
    virtual UINT GetAvailableTextureMem() final;
    virtual HRESULT EvictManagedResources() final;
    virtual HRESULT GetDirect3D(D3D9** ppD3D9) final;
    virtual HRESULT GetDeviceCaps(D3DCAPS9* pCaps) final;
    virtual HRESULT GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) final;
    virtual HRESULT GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* pParameters) final;
    virtual HRESULT SetCursorProperties(UINT XHotSpot, UINT YHotSpot, Surface* pCursorBitmap) final;
    virtual void SetCursorPosition(int X, int Y, DWORD Flags) final;
    virtual BOOL ShowCursor(BOOL bShow) final;
    virtual HRESULT CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) final;
    virtual HRESULT GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) final;
    virtual UINT GetNumberOfSwapChains() final;
    virtual HRESULT Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) final;
    virtual HRESULT Present(const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) final;
    virtual HRESULT GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, Surface** ppBackBuffer) final;
    virtual HRESULT GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) final;
    virtual HRESULT SetDialogBoxMode(BOOL bEnableDialogs) final;
    virtual void SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP* pRamp) final;
    virtual void GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) final;
    virtual HRESULT CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, Texture** ppTexture, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, CubeTexture** ppCubeTexture, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, VertexBuffer** ppVertexBuffer, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IndexBuffer** ppIndexBuffer, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, Surface** ppSurface, HANDLE* pSharedHandle) final;
    virtual HRESULT CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, Surface** ppSurface, HANDLE* pSharedHandle) final;
    virtual HRESULT UpdateSurface(Surface* pSourceSurface, const RECT* pSourceRect, Surface* pDestinationSurface, const POINT* pDestPoint) final;
    virtual HRESULT UpdateTexture(BaseTexture* pSourceTexture, BaseTexture* pDestinationTexture) final;
    virtual HRESULT GetRenderTargetData(Surface* pRenderTarget, Surface* pDestSurface) final;
    virtual HRESULT GetFrontBufferData(UINT iSwapChain, Surface* pDestSurface) final;
    virtual HRESULT StretchRect(Surface* pSourceSurface, const RECT* pSourceRect, Surface* pDestSurface, const RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) final;
    virtual HRESULT ColorFill(Surface* pSurface, const RECT* pRect, D3DCOLOR color) final;
    virtual HRESULT CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, Surface** ppSurface, HANDLE* pSharedHandle) final;
    virtual HRESULT SetRenderTarget(DWORD RenderTargetIndex, Surface* pRenderTarget) final;
    virtual HRESULT GetRenderTarget(DWORD RenderTargetIndex, Surface** ppRenderTarget) final;
    virtual HRESULT SetDepthStencilSurface(Surface* pNewZStencil) final;
    virtual HRESULT GetDepthStencilSurface(Surface** ppZStencilSurface) final;
    virtual HRESULT BeginScene() final;
    virtual HRESULT EndScene() final;
    virtual HRESULT Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) final;
    virtual HRESULT SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix) final;
    virtual HRESULT GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) final;
    virtual HRESULT MultiplyTransform(D3DTRANSFORMSTATETYPE, const D3DMATRIX*) final;
    virtual HRESULT SetViewport(const D3DVIEWPORT9* pViewport) final;
    virtual HRESULT GetViewport(D3DVIEWPORT9* pViewport) final;
    virtual HRESULT SetMaterial(const D3DMATERIAL9* pMaterial) final;
    virtual HRESULT GetMaterial(D3DMATERIAL9* pMaterial) final;
    virtual HRESULT SetLight(DWORD Index, const D3DLIGHT9*) final;
    virtual HRESULT GetLight(DWORD Index, D3DLIGHT9*) final;
    virtual HRESULT LightEnable(DWORD Index, BOOL Enable) final;
    virtual HRESULT GetLightEnable(DWORD Index, BOOL* pEnable) final;
    virtual HRESULT SetClipPlane(DWORD Index, const float* pPlane) final;
    virtual HRESULT GetClipPlane(DWORD Index, float* pPlane) final;
    virtual HRESULT SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) final;
    virtual HRESULT GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) final;
    virtual HRESULT CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) final;
    virtual HRESULT BeginStateBlock() final;
    virtual HRESULT EndStateBlock(IDirect3DStateBlock9** ppSB) final;
    virtual HRESULT SetClipStatus(const D3DCLIPSTATUS9* pClipStatus) final;
    virtual HRESULT GetClipStatus(D3DCLIPSTATUS9* pClipStatus) final;
    virtual HRESULT GetTexture(DWORD Stage, BaseTexture** ppTexture) final;
    virtual HRESULT SetTexture(DWORD Stage, BaseTexture* pTexture) final;
    virtual HRESULT GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) final;
    virtual HRESULT SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) final;
    virtual HRESULT GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) final;
    virtual HRESULT SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) final;
    virtual HRESULT ValidateDevice(DWORD* pNumPasses) final;
    virtual HRESULT SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY* pEntries) final;
    virtual HRESULT GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY* pEntries) final;
    virtual HRESULT SetCurrentTexturePalette(UINT PaletteNumber) final;
    virtual HRESULT GetCurrentTexturePalette(UINT* PaletteNumber) final;
    virtual HRESULT SetScissorRect(const RECT* pRect) final;
    virtual HRESULT GetScissorRect(RECT* pRect) final;
    virtual HRESULT SetSoftwareVertexProcessing(BOOL bSoftware) final;
    virtual BOOL GetSoftwareVertexProcessing() final;
    virtual HRESULT SetNPatchMode(float nSegments) final;
    virtual float GetNPatchMode() final;
    virtual HRESULT DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) final;
    virtual HRESULT DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) final;
    virtual HRESULT DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) final;
    virtual HRESULT DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride) final;
    virtual HRESULT ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, VertexBuffer* pDestBuffer, VertexDeclaration* pVertexDecl, DWORD Flags) final;
    virtual HRESULT CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, VertexDeclaration** ppDecl) final;
    virtual HRESULT SetVertexDeclaration(VertexDeclaration* pDecl) final;
    virtual HRESULT GetVertexDeclaration(VertexDeclaration** ppDecl) final;
    virtual HRESULT SetFVF(DWORD FVF) final;
    virtual HRESULT GetFVF(DWORD* pFVF) final;
    virtual HRESULT CreateVertexShader(const DWORD* pFunction, VertexShader** ppShader, DWORD FunctionSize) final;
    virtual HRESULT SetVertexShader(VertexShader* pShader) final;
    virtual HRESULT GetVertexShader(VertexShader** ppShader) final;
    virtual HRESULT SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT SetVertexShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT GetVertexShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT SetStreamSource(UINT StreamNumber, VertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride) final;
    virtual HRESULT GetStreamSource(UINT StreamNumber, VertexBuffer** ppStreamData, UINT* pOffsetInBytes, UINT* pStride) final;
    virtual HRESULT SetStreamSourceFreq(UINT StreamNumber, UINT Setting) final;
    virtual HRESULT GetStreamSourceFreq(UINT StreamNumber, UINT* pSetting) final;
    virtual HRESULT SetIndices(IndexBuffer* pIndexData) final;
    virtual HRESULT GetIndices(IndexBuffer** ppIndexData) final;
    virtual HRESULT CreatePixelShader(const DWORD* pFunction, PixelShader** ppShader, DWORD FunctionSize) final;
    virtual HRESULT SetPixelShader(PixelShader* pShader) final;
    virtual HRESULT GetPixelShader(PixelShader** ppShader) final;
    virtual HRESULT SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) final;
    virtual HRESULT SetPixelShaderConstantI(UINT StartRegister, const int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) final;
    virtual HRESULT SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT GetPixelShaderConstantB(UINT StartRegister, BOOL* pConstantData, UINT BoolCount) final;
    virtual HRESULT DrawRectPatch(UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo) final;
    virtual HRESULT DrawTriPatch(UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo) final;
    virtual HRESULT DeletePatch(UINT Handle) final;
    virtual HRESULT CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) final;
};
