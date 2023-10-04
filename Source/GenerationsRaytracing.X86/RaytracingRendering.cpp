#include "RaytracingRendering.h"

#include "BottomLevelAccelStruct.h"
#include "Device.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"
#include "TopLevelAccelStruct.h"

static void traverseRenderable(Hedgehog::Mirage::CRenderable* renderable)
{
    if (!renderable->m_Enabled)
        return;

    if (auto element = dynamic_cast<const Hedgehog::Mirage::CSingleElement*>(renderable))
    {
        if (element->m_spModel->IsMadeAll() && (element->m_spInstanceInfo->m_Flags & Hedgehog::Mirage::eInstanceInfoFlags_Invisible) == 0)
        {
            const auto modelDataEx = 
                reinterpret_cast<ModelDataEx*>(element->m_spModel.get());

            if (modelDataEx->m_bottomLevelAccelStructId == NULL)
                return;

            uint32_t bottomLevelAccelStructId = BottomLevelAccelStruct::create(*modelDataEx, nullptr);

            auto& message = s_messageSender.makeMessage<MsgCreateInstance>();

            for (size_t i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 4; j++)
                    message.transform[i][j] = element->m_spInstanceInfo->m_Transform(i, j);
            }

            message.instanceId = reinterpret_cast<InstanceInfoEx*>(element->m_spInstanceInfo.get())->m_instanceId;
            message.bottomLevelAccelStructId = bottomLevelAccelStructId;

            s_messageSender.endMessage();
        }
    }
    else if (const auto bundle = dynamic_cast<const Hedgehog::Mirage::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
            traverseRenderable(it.get());
    }
    else if (const auto optimalBundle = dynamic_cast<const Hedgehog::Mirage::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
            traverseRenderable(it);
    }
}

static FUNCTION_PTR(void, __cdecl, originalSceneRender, 0x652110, void*);

static void __cdecl implOfSceneRender(void* a1)
{
    originalSceneRender(a1);

    const auto device = reinterpret_cast<Device*>((
        **reinterpret_cast<Hedgehog::Mirage::CRenderingDevice***>(static_cast<uint8_t*>(a1) + 16))->m_pD3DDevice);

    const auto& renderScene = Sonic::CGameDocument::GetInstance()->GetWorld()->m_pMember->m_spRenderScene;
    const Hedgehog::Base::CStringSymbol symbols[] = { "Object", "Object_Overlay", "Player" };

    for (const auto& symbol : symbols)
    {
        const auto pair = renderScene->m_BundleMap.find(symbol);
        if (pair == renderScene->m_BundleMap.end())
            continue;

        traverseRenderable(pair->second.get());
    }

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