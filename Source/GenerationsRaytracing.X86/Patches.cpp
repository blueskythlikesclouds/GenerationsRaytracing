// Copy pasted from GenerationsD3D11:
// https://github.com/blueskythlikesclouds/DllMods/blob/master/Source/GenerationsD3D11/Mod/Mod.cpp

#include "Patches.h"

#include "D3D9.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

HOOK(void, __cdecl, LoadPictureData, 0x743DE0,
    hh::mr::CPictureData* pPictureData, const uint8_t* pData, size_t length, hh::mr::CRenderingInfrastructure* pRenderingInfrastructure)
{
    if (pPictureData->m_Flags & hh::db::eDatabaseDataFlags_IsMadeOne)
        return;

    pPictureData->m_pD3DTexture = (DX_PATCH::IDirect3DBaseTexture9*)(new Texture());

    const auto msg = msgSender.start<MsgMakePicture>(length);

    msg->texture = (unsigned int)pPictureData->m_pD3DTexture;
    msg->size = length;

    memcpy(MSG_DATA_PTR(msg), pData, length);

    msgSender.finish();

    pPictureData->m_Type = hh::mr::ePictureType_Texture;
    pPictureData->m_Flags |= hh::db::eDatabaseDataFlags_IsMadeOne;
}

typedef VOID(WINAPI* LPD3DXFILL2D)
(
    Eigen::Vector4f* pOut,
    CONST Eigen::Vector2f* pTexCoord,
    CONST Eigen::Vector2f* pTexelSize,
    LPVOID pData
);

HOOK(HRESULT, __stdcall, FillTexture, 0xA55270, Texture* pTexture, LPD3DXFILL2D pFunction, LPVOID pData)
{
    return E_FAIL;
}

const uint16_t groundSmokeParticleIndexBuffer[] =
{
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5
};

constexpr size_t groundSmokeParticleIndexBufferCount = _countof(groundSmokeParticleIndexBuffer);
constexpr size_t groundSmokeParticleIndexBufferSize = sizeof(groundSmokeParticleIndexBuffer);

// Release mode fails to compile when calling memcpy in inline assembly.
void __stdcall memcpyTrampoline(void* _Dst, void const* _Src, size_t _Size)
{
    memcpy(_Dst, _Src, _Size);
}

void __declspec(naked) groundSmokeParticleCopyIndexBufferMidAsmHook()
{
    static uint32_t returnAddr = 0x11A37CC;

    __asm
    {
        push groundSmokeParticleIndexBufferSize
        push offset groundSmokeParticleIndexBuffer
        push ecx
        call memcpyTrampoline

        jmp[returnAddr]
    }
}

int __stdcall sfdDecodeImpl(int a1, Texture*& texture)
{
    FUNCTION_PTR(int, __cdecl, fun7DFF60, 0x7DFF60, int, int);
    FUNCTION_PTR(int, __cdecl, fun7E08C0, 0x7E08C0, int, int, int);
    FUNCTION_PTR(int, __cdecl, fun7E0BA0, 0x7E0BA0, int, int, void*);

    int v2 = *(int*)(a1 + 296);
    if (!v2)
        return -1;

    fun7DFF60(v2, a1 + 128);

    if (!*(int*)(a1 + 128))
        return -1;

    D3DLOCKED_RECT lockedRect;
    texture->LockRect(0, &lockedRect, nullptr, 0);

    fun7E08C0(*(int*)(a1 + 296), lockedRect.Pitch, *(int*)(a1 + 144));
    fun7E0BA0(*(int*)(a1 + 296), a1 + 128, lockedRect.pBits);

    texture->UnlockRect(0);

    return *(int*)(a1 + 172);
}

void __declspec(naked) sfdDecodeTrampoline()
{
    __asm
    {
        push[esp + 4]
        push esi
        call sfdDecodeImpl
        retn 4
    }
}

HICON __stdcall LoadIconImpl(HINSTANCE hInstance, LPCSTR lpIconName)
{
    return LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(2057));
}

HOOK(D3D9*, __cdecl, Direct3DCreate, 0xA5EDD0, UINT SDKVersion)
{
    return new D3D9();
}

void Patches::init()
{
    INSTALL_HOOK(LoadPictureData);
    INSTALL_HOOK(FillTexture);
    INSTALL_HOOK(Direct3DCreate);

    // Patch the window function to load the icon in the executable.
    WRITE_CALL(0xE7B843, LoadIconImpl);
    WRITE_NOP(0xE7B848, 1);

    // Hide window when it's first created because it's not a pleasant sight to see it centered/resized afterwards.
    WRITE_MEMORY(0xE7B8F7, uint8_t, 0x00);

    // Prevent half-pixel correction
    WRITE_MEMORY(0x64F4C7, uintptr_t, 0x15C5858); // MTFx
    WRITE_MEMORY(0x64CC4B, uintptr_t, 0x15C5858); // ^^
    WRITE_MEMORY(0x7866E2, uintptr_t, 0x15C5858); // FxPipeline
    WRITE_MEMORY(0x64CBBB, uint8_t, 0xD9, 0xEE, 0x90, 0x90, 0x90, 0x90); // Devil's Details

    // Ignore Devil's Details' fullscreen shader
    WRITE_CALL(0x64CF9E, 0x64F470);
    
    // Smoke effect uses triangle fans, patch the index buffer to use triangles instead
    WRITE_MEMORY(0x11A379C, uint8_t, groundSmokeParticleIndexBufferSize);
    WRITE_MEMORY(0x11A37B0, uint8_t, groundSmokeParticleIndexBufferSize);
    WRITE_JUMP(0x11A37C0, groundSmokeParticleCopyIndexBufferMidAsmHook);
    WRITE_MEMORY(0x11A2C63, uint8_t, groundSmokeParticleIndexBufferCount / 3);
    WRITE_MEMORY(0x11A2C6D, uint8_t, D3DPT_TRIANGLELIST);

    // 30 FPS SFDs get decoded at the original frame rate, but get invalidated
    // at the game's frame rate. This is due to Lock/Unlock calls that
    // don't update the returned buffer, which causes flickering.

    // Patch Lock/Unlock calls to call BeginSfdDecodeCallback/EndSfdDecodeCallback instead.
    WRITE_MEMORY(0x5787C8, uint8_t, 0x64);
    WRITE_MEMORY(0x5787FA, uint8_t, 0x68);

    WRITE_MEMORY(0x583DF0, uint8_t, 0x64);
    WRITE_MEMORY(0x583E24, uint8_t, 0x68);

    WRITE_MEMORY(0xB1F184, uint8_t, 0x64);
    WRITE_MEMORY(0xB1F1CF, uint8_t, 0x68);

    WRITE_MEMORY(0x1058FC6, uint8_t, 0x64);
    WRITE_MEMORY(0x1059001, uint8_t, 0x68);

    WRITE_MEMORY(0x1059F92, uint8_t, 0x64);
    WRITE_MEMORY(0x1059FB1, uint8_t, 0x68);

    // Patch SFD decode function to copy data when it's valid.
    WRITE_JUMP(0x1059130, sfdDecodeTrampoline);
}