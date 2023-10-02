#include "RaytracingRendering.h"

#include "Device.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"

static FUNCTION_PTR(void, __cdecl, originalSceneRender, 0x652110, void*);

static void __cdecl implOfSceneRender(void* a1)
{
    originalSceneRender(a1);

    const auto device = reinterpret_cast<Device*>((
        **reinterpret_cast<Hedgehog::Mirage::CRenderingDevice***>(static_cast<uint8_t*>(a1) + 16))->m_pD3DDevice);

    auto& traceRaysMessage = s_messageSender.makeMessage<MsgTraceRays>();

    traceRaysMessage.width = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 4);
    traceRaysMessage.height = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 6);

    s_messageSender.endMessage();
}

void RaytracingRendering::init()
{
    WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2
    WRITE_MEMORY(0x13DDCA0, size_t, 1); // Override game scene child count
    WRITE_MEMORY(0x13DC2D8, void*, &implOfSceneRender); // Override scene render function
    WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
}
