#include "TriangleFan.h"

static const uint16_t groundSmokeParticleIndexBuffer[] =
{
    0, 1, 2,
    0, 2, 3,
    0, 3, 4,
    0, 4, 5
};

static constexpr size_t groundSmokeParticleIndexBufferCount = _countof(groundSmokeParticleIndexBuffer);
static constexpr size_t groundSmokeParticleIndexBufferSize = sizeof(groundSmokeParticleIndexBuffer);

// Release mode fails to compile when calling memcpy in inline assembly.
static void __stdcall memcpyTrampoline(void* _Dst, void const* _Src, size_t _Size)
{
    memcpy(_Dst, _Src, _Size);
}

static void __declspec(naked) groundSmokeParticleCopyIndexBufferMidAsmHook()
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

void TriangleFan::init()
{
    // Smoke effect uses triangle fans, patch the index buffer to use triangles instead
    WRITE_MEMORY(0x11A379C, uint8_t, groundSmokeParticleIndexBufferSize);
    WRITE_MEMORY(0x11A37B0, uint8_t, groundSmokeParticleIndexBufferSize);
    WRITE_JUMP(0x11A37C0, groundSmokeParticleCopyIndexBufferMidAsmHook);
    WRITE_MEMORY(0x11A2C63, uint8_t, groundSmokeParticleIndexBufferCount / 3);
    WRITE_MEMORY(0x11A2C6D, uint8_t, D3DPT_TRIANGLELIST);
}
