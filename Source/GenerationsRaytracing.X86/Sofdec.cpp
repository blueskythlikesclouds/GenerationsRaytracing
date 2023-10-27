#include "Sofdec.h"

#include "Texture.h"

static int __stdcall sfdDecodeImpl(int a1, Texture*& texture)
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

static void __declspec(naked) sfdDecodeTrampoline()
{
    __asm
    {
        push[esp + 4]
        push esi
        call sfdDecodeImpl
        retn 4
    }
}

void Sofdec::init()
{
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
