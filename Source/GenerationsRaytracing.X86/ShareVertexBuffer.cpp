#include "ShareVertexBuffer.h"
#include "SampleChunkResource.h"

static FUNCTION_PTR(void, __thiscall, shareVertexBufferCtor, 0x72EC90, void*);
static FUNCTION_PTR(void, __thiscall, shareVertexBufferDtor, 0x72EBE0, void*);
static FUNCTION_PTR(void, __thiscall, shareVertexBufferSetRenderingInfrastructure, 0x72E8D0, void*, Hedgehog::Mirage::CRenderingInfrastructure*);
static FUNCTION_PTR(void, __cdecl, shareVertexBufferStart, 0x725000, void*, void*);
static FUNCTION_PTR(void, __cdecl, shareVertexBufferFinish, 0x725050, void*, void*);

HOOK(void, __cdecl, ModelDataMake, 0x7337A0,
    const Hedgehog::Base::CSharedString& name,
    void* data,
    uint32_t dataSize,
    const boost::shared_ptr<Hedgehog::Database::CDatabase>& database,
    Hedgehog::Mirage::CRenderingInfrastructure* renderingInfrastructure)
{
    ShareVertexBuffer::s_makingModelData = true;

    if (data != nullptr)
    {
        ShareVertexBuffer::s_loadingSampleChunkV2 =
            (_byteswap_ulong(reinterpret_cast<const SampleChunkHeaderV2*>(data)->fileSize) & 0x80000000) != 0;

        if (ShareVertexBuffer::s_loadingSampleChunkV2)
            reinterpret_cast<SampleChunkHeaderV1*>(data)->version = 0x05000000;
    }

    if (*reinterpret_cast<uint32_t*>(0x1B244D4) == NULL)
    {
        uint32_t shareVertexBuffer[0x30 / sizeof(uint32_t)];
        shareVertexBufferCtor(shareVertexBuffer);
        shareVertexBufferSetRenderingInfrastructure(shareVertexBuffer, renderingInfrastructure);
        shareVertexBufferStart(shareVertexBuffer, nullptr);
        originalModelDataMake(name, data, dataSize, database, renderingInfrastructure);
        shareVertexBufferFinish(shareVertexBuffer, nullptr);
        shareVertexBufferDtor(shareVertexBuffer);
    }
    else
    {
        originalModelDataMake(name, data, dataSize, database, renderingInfrastructure);
    }

    ShareVertexBuffer::s_loadingSampleChunkV2 = false;
    ShareVertexBuffer::s_makingModelData = false;
}

static void __declspec(naked) meshDataDrawIndexedPrimitiveMidAsmHook()
{
    __asm
    {
        mov ebp, [edi + 0x24]
        mov edi, [edi + 0xC]
        mov esi, [esi + 0x4]
        mov edx, [esi]
        mov edx, [edx + 0x14C]
        lea eax, [edi - 2]
        push eax
        push ebp
        push edi
        push 0
        push 0
        push 5
        mov ecx, esi
        call edx
        pop edi
        pop esi
        pop ebp
        pop ebx
        retn
    }
}

void ShareVertexBuffer::init()
{
    INSTALL_HOOK(ModelDataMake);

    // Fix game not using index offset when drawing models
    WRITE_JUMP(0x6FD572, meshDataDrawIndexedPrimitiveMidAsmHook);

    // Fix game overallocating index buffers
    WRITE_NOP(0x72EA3F, 3);
}
