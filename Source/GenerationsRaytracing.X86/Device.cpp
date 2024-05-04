#include "Device.h"

#include "Configuration.h"
#include "IndexBuffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "PixelShader.h"
#include "RaytracingParams.h"
#include "Surface.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "VertexDeclaration.h"
#include "VertexShader.h"
#include "RaytracingUtil.h"

void Device::createVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, VertexDeclaration** ppDecl, bool isFVF)
{
    auto vertexElement = pVertexElements;
    size_t vertexElementCount = 0;

    while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
    {
        ++vertexElement;
        ++vertexElementCount;
    }

    auto& pDecl = m_vertexDeclarationMap[
        XXH32(pVertexElements, sizeof(D3DVERTEXELEMENT9) * vertexElementCount, isFVF)];

    if (!pDecl)
    {
        pDecl.Attach(new VertexDeclaration(pVertexElements));

        auto& message = s_messageSender.makeMessage<MsgCreateVertexDeclaration>(
            pDecl->getVertexElementsSize() * sizeof(D3DVERTEXELEMENT9));

        message.vertexDeclarationId = pDecl->getId();
        message.isFVF = isFVF;
        memcpy(message.data, pDecl->getVertexElements(), pDecl->getVertexElementsSize() * sizeof(D3DVERTEXELEMENT9));

        s_messageSender.endMessage();
    }

    pDecl.CopyTo(ppDecl);
}

void Device::initImgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplWin32_Init(m_hWnd);

    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "GenerationsRaytracing";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    WCHAR winDir[MAX_PATH];
    GetWindowsDirectoryW(winDir, MAX_PATH);

    char fontFile[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, winDir, -1, fontFile, _countof(fontFile), nullptr, nullptr);
    strcat_s(fontFile, "\\Fonts\\segoeui.ttf");

    if (!io.Fonts->AddFontFromFileTTF(fontFile, 18.0f))
        io.Fonts->AddFontDefault();

    io.Fonts->Build();

    static constexpr D3DVERTEXELEMENT9 VERTEX_ELEMENTS[] =
    {
        { 0, offsetof(ImDrawVert, pos), D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_POSITION, 0 },
        { 0, offsetof(ImDrawVert, uv), D3DDECLTYPE_FLOAT2, 0, D3DDECLUSAGE_TEXCOORD, 0 },
        { 0, offsetof(ImDrawVert, col), D3DDECLTYPE_UBYTE4N, 0, D3DDECLUSAGE_COLOR, 0 },
        D3DDECL_END()
    };

    createVertexDeclaration(VERTEX_ELEMENTS, m_imgui.vertexDeclaration.GetAddressOf(), false);

    uint8_t* pixels;
    int width, height, bpp;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bpp);

    CreateTexture(width, height, 1, NULL, D3DFMT_A8B8G8R8, D3DPOOL_DEFAULT, m_imgui.fontTexture.GetAddressOf(), nullptr);

    D3DLOCKED_RECT lockedRect{};
    m_imgui.fontTexture->LockRect(0, &lockedRect, nullptr, 0);

    if (width * bpp == lockedRect.Pitch)
    {
        memcpy(lockedRect.pBits, pixels, width * height * bpp);
    }
    else
    {
        const size_t pitch = width * bpp;

        for (int i = 0; i < height; i++)
            memcpy(static_cast<uint8_t*>(lockedRect.pBits) + lockedRect.Pitch * i, pixels + pitch * i, pitch);
    }

    m_imgui.fontTexture->UnlockRect(0);
    io.Fonts->SetTexID(m_imgui.fontTexture.Get());
}

void Device::beginImgui()
{
    if (!m_imgui.init)
    {
        initImgui();
        m_imgui.init = true;
    }

    m_imgui.render = true;

    ImGui_ImplWin32_NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = { static_cast<float>(m_backBuffer->getWidth()),
        static_cast<float>(m_backBuffer->getHeight()) };

    ImGui::NewFrame();
}

void Device::endImgui()
{
    if (m_imgui.render)
    {
        ImGui::Render();
        m_imgui.render = false;
    }
}

void Device::renderImgui()
{
    ImDrawData* drawData = ImGui::GetDrawData();

    const size_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    const size_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    if (vertexBufferSize == 0 || indexBufferSize == 0)
        return;

    Hedgehog::Mirage::CMirageDatabaseWrapper wrapper(
        Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spShaderDatabase.get());

    const auto csdVS = wrapper.GetVertexShaderCodeData("csd");
    const auto csdPS = wrapper.GetPixelShaderCodeData("csd");
    const auto csdNoTexVS = wrapper.GetVertexShaderCodeData("csdNoTex");
    const auto csdNoTexPS = wrapper.GetPixelShaderCodeData("csdNoTex");

    if (!csdVS || !csdVS->IsMadeAll() ||
        !csdPS || !csdPS->IsMadeAll() ||
        !csdNoTexVS || !csdNoTexVS->IsMadeAll() ||
        !csdNoTexPS || !csdNoTexPS->IsMadeAll())
    {
        return;
    }

    if (!m_imgui.vertexBuffer || m_imgui.vertexBuffer->getByteSize() < vertexBufferSize)
        CreateVertexBuffer(vertexBufferSize, D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, m_imgui.vertexBuffer.ReleaseAndGetAddressOf(), nullptr);

    if (!m_imgui.indexBuffer || m_imgui.indexBuffer->getByteSize() < indexBufferSize)
        CreateIndexBuffer(indexBufferSize, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, m_imgui.indexBuffer.ReleaseAndGetAddressOf(), nullptr);

    ImDrawVert* vtx = nullptr;
    ImDrawIdx* idx = nullptr;

    m_imgui.vertexBuffer->Lock(0, vertexBufferSize, reinterpret_cast<void**>(&vtx), D3DLOCK_DISCARD);
    m_imgui.indexBuffer->Lock(0, indexBufferSize, reinterpret_cast<void**>(&idx), D3DLOCK_DISCARD);

    for (int i = 0; i < drawData->CmdListsCount; i++)
    {
        const auto drawList = drawData->CmdLists[i];

        const ImDrawVert* srcVtx = drawList->VtxBuffer.Data;
        for (int j = 0; j < drawList->VtxBuffer.Size; j++)
        {
            const uint8_t r = (srcVtx->col >> IM_COL32_R_SHIFT) & 0xFF;
            const uint8_t g = (srcVtx->col >> IM_COL32_G_SHIFT) & 0xFF;
            const uint8_t b = (srcVtx->col >> IM_COL32_B_SHIFT) & 0xFF;
            const uint8_t a = (srcVtx->col >> IM_COL32_A_SHIFT) & 0xFF;

            // wxyz: rgba
            // x: g
            // y: b
            // z: a
            // w: r
            vtx->col = (g << 0) | (b << 8) | (a << 16) | (r << 24);

            vtx->pos[0] = srcVtx->pos.x + 0.5f;
            vtx->pos[1] = srcVtx->pos.y + 0.5f;
            vtx->uv[0] = srcVtx->uv.x;
            vtx->uv[1] = srcVtx->uv.y;

            vtx++;
            srcVtx++;
        }

        memcpy(idx, drawList->IdxBuffer.Data, drawList->IdxBuffer.size_in_bytes());
        idx += drawList->IdxBuffer.Size;
    }

    m_imgui.indexBuffer->Unlock();
    m_imgui.vertexBuffer->Unlock();

    D3DVIEWPORT9 viewport{};
    viewport.Width = static_cast<DWORD>(drawData->DisplaySize.x);
    viewport.Height = static_cast<DWORD>(drawData->DisplaySize.y);
    viewport.MaxZ = 1.0f;

    SetViewport(&viewport);
    SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    SetRenderState(D3DRS_ZENABLE, FALSE);
    SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
    SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
    SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    SetVertexDeclaration(m_imgui.vertexDeclaration.Get());
    SetStreamSource(0, m_imgui.vertexBuffer.Get(), 0, sizeof(ImDrawVert));
    SetIndices(m_imgui.indexBuffer.Get());

    const float viewportSize[] = { drawData->DisplaySize.x, drawData->DisplaySize.y, 1.0f / drawData->DisplaySize.x, 1.0f / drawData->DisplaySize.y };
    constexpr float scaleSize[] = { 1.0f, 1.0f, 0.0f, 0.0f };
    constexpr float z[] = { 0.0f, 0.0f, 0.0f, 0.0f };

    SetVertexShaderConstantF(180, viewportSize, 1);
    SetVertexShaderConstantF(246, z, 1);
    SetVertexShaderConstantF(247, scaleSize, 1);

    const ImVec2 clipOffset = drawData->DisplayPos;
    size_t vtxOffset = 0;
    size_t idxOffset = 0;

    for (int i = 0; i < drawData->CmdListsCount; i++)
    {
        const ImDrawList* cmdList = drawData->CmdLists[i];

        for (int j = 0; j < cmdList->CmdBuffer.Size; j++)
        {
            const ImDrawCmd& cmd = cmdList->CmdBuffer[j];

            ImVec2 clipMin(cmd.ClipRect.x - clipOffset.x, cmd.ClipRect.y - clipOffset.y);
            ImVec2 clipMax(cmd.ClipRect.z - clipOffset.x, cmd.ClipRect.w - clipOffset.y);

            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                continue;

            const RECT scissorRect =
            {
                static_cast<LONG>(clipMin.x),
                static_cast<LONG>(clipMin.y),
                static_cast<LONG>(clipMax.x),
                static_cast<LONG>(clipMax.y)
            };

            const bool hasTexture = cmd.GetTexID() != nullptr;

            SetVertexShader(reinterpret_cast<VertexShader*>(hasTexture ? csdVS->m_pD3DVertexShader : csdNoTexVS->m_pD3DVertexShader));
            SetPixelShader(reinterpret_cast<PixelShader*>(hasTexture ? csdPS->m_pD3DPixelShader : csdNoTexPS->m_pD3DPixelShader));

            SetTexture(0, static_cast<BaseTexture*>(cmd.GetTexID()));

            SetScissorRect(&scissorRect);
            DrawIndexedPrimitive(D3DPT_TRIANGLELIST, cmd.VtxOffset + vtxOffset, 0, static_cast<UINT>(cmdList->VtxBuffer.Size), cmd.IdxOffset + idxOffset, cmd.ElemCount / 3);
        }

        idxOffset += cmdList->IdxBuffer.Size;
        vtxOffset += cmdList->VtxBuffer.Size;
    }
}

void Device::beginIm3d()
{
    auto& appData = Im3d::GetAppData();
    m_im3d.render = true;

    appData.m_viewportSize.x = static_cast<float>(m_backBuffer->getWidth());
    appData.m_viewportSize.y = static_cast<float>(m_backBuffer->getHeight());

    if (const auto gameDocument = Sonic::CGameDocument::GetInstance())
    {
        const auto camera = gameDocument->GetWorld()->GetCamera();

        memcpy(m_im3d.projection, camera->m_MyCamera.m_Projection.data(), sizeof(m_im3d.projection));
        memcpy(m_im3d.view, camera->m_MyCamera.m_View.data(), sizeof(m_im3d.view));

        Sonic::CNoAlignVector direction;
        direction.x() = (ImGui::GetMousePos().x / appData.m_viewportSize.x * 2.0f - 1.0f) / camera->m_MyCamera.m_Projection(0, 0);
        direction.y() = (ImGui::GetMousePos().y / appData.m_viewportSize.y * 2.0f - 1.0f) / -camera->m_MyCamera.m_Projection(1, 1);
        direction.z() = -1.0f;
        direction = (camera->m_MyCamera.m_View.rotation().inverse() * direction).normalized();

        appData.m_cursorRayOrigin.x = camera->m_MyCamera.m_Position.x();
        appData.m_cursorRayOrigin.y = camera->m_MyCamera.m_Position.y();
        appData.m_cursorRayOrigin.z = camera->m_MyCamera.m_Position.z();
        appData.m_cursorRayDirection.x = direction.x();
        appData.m_cursorRayDirection.y = direction.y();
        appData.m_cursorRayDirection.z = direction.z();
        appData.m_viewOrigin.x = camera->m_MyCamera.m_Position.x();
        appData.m_viewOrigin.y = camera->m_MyCamera.m_Position.y();
        appData.m_viewOrigin.z = camera->m_MyCamera.m_Position.z();
        appData.m_viewDirection.x = camera->m_MyCamera.m_Direction.x();
        appData.m_viewDirection.y = camera->m_MyCamera.m_Direction.y();
        appData.m_viewDirection.z = camera->m_MyCamera.m_Direction.z();
        appData.m_projScaleY = 4.0f * (1.0f / camera->m_MyCamera.m_Projection(1, 1));
    }

    Im3d::NewFrame();
}

void Device::endIm3d()
{
    if (m_im3d.render)
    {
        Im3d::EndFrame();
        m_im3d.render = false;
    }
}

void Device::renderIm3d()
{
    const uint32_t drawListCount = Im3d::GetDrawListCount();
    if (drawListCount == 0)
        return;

    uint32_t vertexSize = 0;
    for (size_t i = 0; i < drawListCount; i++)
        vertexSize += Im3d::GetDrawLists()[i].m_vertexCount * sizeof(Im3d::VertexData);

    auto& message = s_messageSender.makeMessage<MsgDrawIm3d>(
        vertexSize + sizeof(MsgDrawIm3d::DrawList) * drawListCount);

    memcpy(message.projection, m_im3d.projection, sizeof(message.projection));
    memcpy(message.view, m_im3d.view, sizeof(message.view));
    message.viewportSize[0] = Im3d::GetAppData().m_viewportSize.x;
    message.viewportSize[1] = Im3d::GetAppData().m_viewportSize.y;
    message.vertexSize = vertexSize;
    message.drawListCount = drawListCount;

    auto dstDrawList = reinterpret_cast<MsgDrawIm3d::DrawList*>(message.data + vertexSize);
    uint32_t vertexOffset = 0;

    for (size_t i = 0; i < drawListCount; i++)
    {
        auto& drawList = Im3d::GetDrawLists()[i];

        dstDrawList->primitiveType = static_cast<uint8_t>(drawList.m_primType);
        dstDrawList->vertexOffset = vertexOffset;
        dstDrawList->vertexCount = drawList.m_vertexCount;

        memcpy(message.data + vertexOffset, drawList.m_vertexData,
            sizeof(Im3d::VertexData) * drawList.m_vertexCount);

        ++dstDrawList;
        vertexOffset += sizeof(Im3d::VertexData) * drawList.m_vertexCount;
    }

    s_messageSender.endMessage();
}

Device::Device(uint32_t width, uint32_t height, HWND hWnd)
    : m_hWnd(hWnd)
{
    m_backBuffer.Attach(new Texture(width, height, 1));

    if (Configuration::s_hdr)
    {
        CreateTexture(
            width,
            height,
            1,
            D3DUSAGE_RENDERTARGET,
            D3DFMT_A16B16G16R16F,
            D3DPOOL_DEFAULT,
            m_hdrTexture.GetAddressOf(),
            nullptr);
    }
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
    if (Configuration::s_enableImgui)
    {
        renderIm3d();
        renderImgui();
    }

    if (Configuration::s_hdr)
    {
        // Force creation of RTV/SRV
        SetRenderTarget(0, m_backBuffer->getSurface(0));
        SetTexture(0, m_hdrTexture.Get());

        s_messageSender.makeMessage<MsgCopyHdrTexture>();
        s_messageSender.endMessage();
    }

    s_messageSender.makeMessage<MsgPresent>();
    s_messageSender.endMessage();

    s_messageSender.commitMessages();

    return S_OK;
}

HRESULT Device::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, Surface** ppBackBuffer)
{
    return Configuration::s_hdr ? m_hdrTexture->GetSurfaceLevel(0, ppBackBuffer) : m_backBuffer->GetSurfaceLevel(0, ppBackBuffer);
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
    if (Format == MAKEFOURCC('N', 'U', 'L', 'L'))
    {
        *ppTexture = nullptr;
    }
    else
    {
        if ((Usage & D3DUSAGE_RENDERTARGET) && (Format == D3DFMT_A8R8G8B8 || Format == D3DFMT_A8B8G8R8))
            Format = D3DFMT_A16B16G16R16F;

        *ppTexture = new Texture(Width, Height, Levels);

        auto& message = s_messageSender.makeMessage<MsgCreateTexture>();

        message.width = Width;
        message.height = Height;
        message.levels = Levels;
        message.usage = Usage;
        message.format = Format;
        message.textureId = (*ppTexture)->getId();

        s_messageSender.endMessage();
    }

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateVolumeTexture, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateCubeTexture, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, CubeTexture** ppCubeTexture, HANDLE* pSharedHandle)

HRESULT Device::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, VertexBuffer** ppVertexBuffer, HANDLE* pSharedHandle)
{
    *ppVertexBuffer = new VertexBuffer(Length);

    auto& message = s_messageSender.makeMessage<MsgCreateVertexBuffer>();

    message.length = Length;
    message.vertexBufferId = (*ppVertexBuffer)->getId();
    message.allowUnorderedAccess = false;

    s_messageSender.endMessage();

    return S_OK;
}

HRESULT Device::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IndexBuffer** ppIndexBuffer, HANDLE* pSharedHandle)
{
    *ppIndexBuffer = new IndexBuffer(Length);

    auto& message = s_messageSender.makeMessage<MsgCreateIndexBuffer>();

    message.length = Length;
    message.format = Format;
    message.indexBufferId = (*ppIndexBuffer)->getId();

    s_messageSender.endMessage();

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
        auto& message = s_messageSender.makeMessage<MsgSetRenderTarget>();

        message.renderTargetIndex = static_cast<uint8_t>(RenderTargetIndex);
        message.textureId = texture != nullptr ? texture->getId() : NULL;
        message.textureLevel = static_cast<uint8_t>(level);

        s_messageSender.endMessage();

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
        auto& message = s_messageSender.makeMessage<MsgSetDepthStencilSurface>();

        message.depthStencilSurfaceId = texture != nullptr ? texture->getId() : NULL;
        message.level = level;

        s_messageSender.endMessage();

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
    if (Configuration::s_enableImgui != m_imgui.enable)
    {
        auto& message = s_messageSender.makeMessage<MsgShowCursor>();
        message.showCursor = Configuration::s_enableImgui;
        s_messageSender.endMessage();

        m_imgui.enable = Configuration::s_enableImgui;
    }

    if (Configuration::s_enableImgui)
    {
        beginImgui();
        beginIm3d();

        RaytracingParams::imguiWindow();
    }

    return S_OK;
}

HRESULT Device::EndScene()
{
    endImgui();
    endIm3d();
    RaytracingUtil::releaseResources();

    return S_OK;
}

HRESULT Device::Clear(DWORD Count, const D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    auto& message = s_messageSender.makeMessage<MsgClear>();

    message.flags = Flags;
    message.color = Color;
    message.z = Z;
    message.stencil = static_cast<uint8_t>(Stencil);

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetTransform, D3DTRANSFORMSTATETYPE State, const D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetTransform, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::MultiplyTransform, D3DTRANSFORMSTATETYPE, const D3DMATRIX*)

HRESULT Device::SetViewport(const D3DVIEWPORT9* pViewport)
{
    if (memcmp(&viewport, pViewport, sizeof(D3DVIEWPORT9)) != 0)
    {
        auto& message = s_messageSender.makeMessage<MsgSetViewport>();

        message.x = static_cast<uint16_t>(pViewport->X);
        message.y = static_cast<uint16_t>(pViewport->Y);
        message.width = static_cast<uint16_t>(pViewport->Width);
        message.height = static_cast<uint16_t>(pViewport->Height);
        message.minZ = pViewport->MinZ;
        message.maxZ = pViewport->MaxZ;

        s_messageSender.endMessage();

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
        switch (State)
        {
        case D3DRS_ZENABLE:
        case D3DRS_FILLMODE:
        case D3DRS_ZWRITEENABLE:
        case D3DRS_ALPHATESTENABLE:
        case D3DRS_SRCBLEND:
        case D3DRS_DESTBLEND:
        case D3DRS_CULLMODE:
        case D3DRS_ZFUNC:
        case D3DRS_ALPHAREF:
        case D3DRS_ALPHABLENDENABLE:
        case D3DRS_COLORWRITEENABLE:
        case D3DRS_BLENDOP:
        case D3DRS_SCISSORTESTENABLE:
        case D3DRS_SLOPESCALEDEPTHBIAS:
        case D3DRS_COLORWRITEENABLE1:
        case D3DRS_COLORWRITEENABLE2:
        case D3DRS_COLORWRITEENABLE3:
        case D3DRS_DEPTHBIAS:
        case D3DRS_SRCBLENDALPHA:
        case D3DRS_DESTBLENDALPHA:
        case D3DRS_BLENDOPALPHA:
        {
            auto& message = s_messageSender.makeMessage<MsgSetRenderState>();

            message.state = State;
            message.value = Value;

            s_messageSender.endMessage();

            break;
        }
        }

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
        auto& message = s_messageSender.makeMessage<MsgSetTexture>();

        message.stage = static_cast<uint8_t>(Stage);
        message.textureId = pTexture != nullptr ? pTexture->getId() : NULL;

        s_messageSender.endMessage();

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
        auto& message = s_messageSender.makeMessage<MsgSetSamplerState>();

        message.sampler = static_cast<uint8_t>(Sampler);
        message.type = static_cast<uint8_t>(Type);
        message.value = Value;

        s_messageSender.endMessage();

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
        auto& message = s_messageSender.makeMessage<MsgSetScissorRect>();

        message.left = static_cast<uint16_t>(pRect->left);
        message.top = static_cast<uint16_t>(pRect->top);
        message.right = static_cast<uint16_t>(pRect->right);
        message.bottom = static_cast<uint16_t>(pRect->bottom);

        s_messageSender.endMessage();

        m_scissorRect = *pRect;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetScissorRect, RECT* pRect)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetSoftwareVertexProcessing, BOOL bSoftware)

FUNCTION_STUB(BOOL, FALSE, Device::GetSoftwareVertexProcessing)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetNPatchMode, float nSegments)

FUNCTION_STUB(float, 0.0f, Device::GetNPatchMode)

UINT calculatePrimitiveElements(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
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
    auto& message = s_messageSender.makeMessage<MsgDrawPrimitive>();

    message.primitiveType = PrimitiveType;
    message.startVertex = StartVertex;
    message.vertexCount = calculatePrimitiveElements(PrimitiveType, PrimitiveCount);

    s_messageSender.endMessage();

    return S_OK;
}

HRESULT Device::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    auto& message = s_messageSender.makeMessage<MsgDrawIndexedPrimitive>();

    message.primitiveType = PrimitiveType;
    message.baseVertexIndex = BaseVertexIndex;
    message.startIndex = startIndex;
    message.indexCount = calculatePrimitiveElements(PrimitiveType, primCount);

    s_messageSender.endMessage();

    return S_OK;
}

HRESULT Device::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    const uint32_t vertexCount = calculatePrimitiveElements(PrimitiveType, PrimitiveCount);

    auto& message = s_messageSender.makeMessage<MsgDrawPrimitiveUP>(vertexCount * VertexStreamZeroStride);

    message.primitiveType = PrimitiveType;
    message.vertexCount = vertexCount;
    message.vertexStreamZeroStride = VertexStreamZeroStride;
    memcpy(message.data, pVertexStreamZeroData, vertexCount * VertexStreamZeroStride);

    s_messageSender.endMessage();

    return S_OK;
}

HRESULT Device::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void* pIndexData, D3DFORMAT IndexDataFormat, const void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    const uint32_t indexCount = calculatePrimitiveElements(PrimitiveType, PrimitiveCount);

    const uint32_t verticesSize = VertexStreamZeroStride * NumVertices;
    const uint32_t indicesSize = (IndexDataFormat == D3DFMT_INDEX32 ? 4 : 2) * indexCount;

    auto& message = s_messageSender.makeMessage<MsgDrawIndexedPrimitiveUP>(verticesSize + indicesSize);

    message.primitiveType = static_cast<uint8_t>(PrimitiveType);
    message.vertexCount = NumVertices;
    message.indexCount = indexCount;
    message.indexFormat = static_cast<uint8_t>(IndexDataFormat);
    message.vertexStride = VertexStreamZeroStride;

    memcpy(message.data, pVertexStreamZeroData, verticesSize);
    memcpy(message.data + verticesSize, pIndexData, indicesSize);

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::ProcessVertices, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, VertexBuffer* pDestBuffer, VertexDeclaration* pVertexDecl, DWORD Flags)

HRESULT Device::CreateVertexDeclaration(const D3DVERTEXELEMENT9* pVertexElements, VertexDeclaration** ppDecl)
{
    createVertexDeclaration(pVertexElements, ppDecl, false);
    return S_OK;
}

HRESULT Device::SetVertexDeclaration(VertexDeclaration* pDecl)
{
    if (m_vertexDeclaration.Get() != pDecl)
    {
        auto& message = s_messageSender.makeMessage<MsgSetVertexDeclaration>();
        message.vertexDeclarationId = pDecl != nullptr ? pDecl->getId() : NULL;
        s_messageSender.endMessage();

        m_vertexDeclaration = pDecl;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexDeclaration, VertexDeclaration** ppDecl)

HRESULT Device::SetFVF(DWORD FVF)
{
    auto& vertexDeclaration = m_fvfMap[FVF];
    if (!vertexDeclaration)
    {
        std::vector<D3DVERTEXELEMENT9> vertexElements;
        WORD offset = 0;

        if (FVF & D3DFVF_XYZRHW)
        {
            vertexElements.push_back({ 0, offset, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 });
            offset += 16;
        }

        if (FVF & D3DFVF_DIFFUSE)
        {
            vertexElements.push_back({ 0, offset, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 });
            offset += 4;
        }

        if (FVF & D3DFVF_TEX1)
        {
            vertexElements.push_back({ 0, offset, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 });
            offset += 8;
        }

        vertexElements.push_back(D3DDECL_END());
        createVertexDeclaration(vertexElements.data(), vertexDeclaration.GetAddressOf(), true);
    }

    return SetVertexDeclaration(vertexDeclaration.Get());
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetFVF, DWORD* pFVF)

HRESULT Device::CreateVertexShader(const DWORD* pFunction, VertexShader** ppShader, DWORD FunctionSize)
{
    auto& pShader = m_vertexShaderMap[XXH32(pFunction, FunctionSize, 0)];
    if (!pShader)
    {
        pShader.Attach(new VertexShader());

        auto& message = s_messageSender.makeMessage<MsgCreateVertexShader>(FunctionSize);

        message.vertexShaderId = pShader->getId();
        memcpy(message.data, pFunction, FunctionSize);

        s_messageSender.endMessage();
    }

    pShader.CopyTo(ppShader);

    return S_OK;
}

HRESULT Device::SetVertexShader(VertexShader* pShader)
{
    if (m_vertexShader.Get() != pShader)
    {
        auto& message = s_messageSender.makeMessage<MsgSetVertexShader>();
        message.vertexShaderId = pShader != nullptr ? pShader->getId() : NULL;
        s_messageSender.endMessage();

        m_vertexShader = pShader;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShader, VertexShader** ppShader)

HRESULT Device::SetVertexShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount)
{
    auto& message = s_messageSender.makeMessage<MsgSetVertexShaderConstantF>(Vector4fCount * sizeof(float[4]));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, Vector4fCount * sizeof(float[4]));

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetVertexShaderConstantI, UINT StartRegister, const int* pConstantData, UINT Vector4iCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetVertexShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    auto& message = s_messageSender.makeMessage<MsgSetVertexShaderConstantB>(BoolCount * sizeof(BOOL));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, BoolCount * sizeof(BOOL));

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetVertexShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

HRESULT Device::SetStreamSource(UINT StreamNumber, VertexBuffer* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    if (m_streamData[StreamNumber].Get() != pStreamData ||
        m_offsetsInBytes[StreamNumber] != OffsetInBytes ||
        m_strides[StreamNumber] != Stride)
    {
        auto& message = s_messageSender.makeMessage<MsgSetStreamSource>();

        message.streamNumber = StreamNumber;
        message.streamDataId = pStreamData != nullptr ? pStreamData->getId() : NULL;
        message.offsetInBytes = OffsetInBytes;
        message.stride = Stride;

        s_messageSender.endMessage();

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
        if (StreamNumber == 0)
        {
            auto& message = s_messageSender.makeMessage<MsgSetStreamSourceFreq>();

            message.streamNumber = StreamNumber;
            message.setting = Setting;

            s_messageSender.endMessage();
        }

        m_settings[StreamNumber] = Setting;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetStreamSourceFreq, UINT StreamNumber, UINT* pSetting)

HRESULT Device::SetIndices(IndexBuffer* pIndexData)
{
    if (m_indexData.Get() != pIndexData)
    {
        auto& message = s_messageSender.makeMessage<MsgSetIndices>();
        message.indexDataId = pIndexData != nullptr ? pIndexData->getId() : NULL;
        s_messageSender.endMessage();

        m_indexData = pIndexData;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetIndices, IndexBuffer** ppIndexData)

HRESULT Device::CreatePixelShader(const DWORD* pFunction, PixelShader** ppShader, DWORD FunctionSize)
{
    auto& pShader = m_pixelShaderMap[XXH32(pFunction, FunctionSize, 0)];
    if (!pShader)
    {
        pShader.Attach(new PixelShader());

        auto& message = s_messageSender.makeMessage<MsgCreatePixelShader>(FunctionSize);

        message.pixelShaderId = pShader->getId();
        memcpy(message.data, pFunction, FunctionSize);

        s_messageSender.endMessage();
    }

    pShader.CopyTo(ppShader);

    return S_OK;
}

HRESULT Device::SetPixelShader(PixelShader* pShader)
{
    if (m_pixelShader.Get() != pShader)
    {
        auto& message = s_messageSender.makeMessage<MsgSetPixelShader>();
        message.pixelShaderId = pShader != nullptr ? pShader->getId() : NULL;
        s_messageSender.endMessage();

        m_pixelShader = pShader;
    }
    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShader, PixelShader** ppShader)

HRESULT Device::SetPixelShaderConstantF(UINT StartRegister, const float* pConstantData, UINT Vector4fCount)
{
    auto& message = s_messageSender.makeMessage<MsgSetPixelShaderConstantF>(Vector4fCount * sizeof(float[4]));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, Vector4fCount * sizeof(float[4]));

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantF, UINT StartRegister, float* pConstantData, UINT Vector4fCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::SetPixelShaderConstantI, UINT StartRegister, const int* pConstantData, UINT Vector4iCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantI, UINT StartRegister, int* pConstantData, UINT Vector4iCount)

HRESULT Device::SetPixelShaderConstantB(UINT StartRegister, const BOOL* pConstantData, UINT BoolCount)
{
    auto& message = s_messageSender.makeMessage<MsgSetPixelShaderConstantB>(BoolCount * sizeof(BOOL));

    message.startRegister = StartRegister;
    memcpy(message.data, pConstantData, BoolCount * sizeof(BOOL));

    s_messageSender.endMessage();

    return S_OK;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::GetPixelShaderConstantB, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawRectPatch, UINT Handle, const float* pNumSegs, const D3DRECTPATCH_INFO* pRectPatchInfo)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DrawTriPatch, UINT Handle, const float* pNumSegs, const D3DTRIPATCH_INFO* pTriPatchInfo)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::DeletePatch, UINT Handle)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Device::CreateQuery, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)