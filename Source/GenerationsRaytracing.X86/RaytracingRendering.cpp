#include "RaytracingRendering.h"

#include "ModelData.h"
#include "Device.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"
#include "InstanceData.h"

static void createInstancesAndBottomLevelAccelStructs(Hedgehog::Mirage::CRenderable* renderable, bool isEnabled)
{
    isEnabled &= renderable->m_Enabled;

    if (auto element = dynamic_cast<const Hedgehog::Mirage::CSingleElement*>(renderable))
    {
        if (element->m_spModel->IsMadeAll())
        {
            ModelData::createBottomLevelAccelStruct(
                *reinterpret_cast<ModelDataEx*>(element->m_spModel.get()),
                *reinterpret_cast<InstanceInfoEx*>(element->m_spInstanceInfo.get()),
                element->m_MaterialMap,
                isEnabled && (element->m_spInstanceInfo->m_Flags & Hedgehog::Mirage::eInstanceInfoFlags_Invisible) == 0);
        }
    }
    else if (const auto bundle = dynamic_cast<const Hedgehog::Mirage::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
            createInstancesAndBottomLevelAccelStructs(it.get(), isEnabled);
    }
    else if (const auto optimalBundle = dynamic_cast<const Hedgehog::Mirage::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
            createInstancesAndBottomLevelAccelStructs(it, isEnabled);
    }
}

static void renderSky(Hedgehog::Mirage::CRenderable* renderable)
{
    if (!renderable->m_Enabled)
        return;

    if (auto element = dynamic_cast<const Hedgehog::Mirage::CSingleElement*>(renderable))
    {
        if (element->m_spModel->IsMadeAll() && 
            (element->m_spInstanceInfo->m_Flags & Hedgehog::Mirage::eInstanceInfoFlags_Invisible) == 0)
        {
            ModelData::renderSky(*element->m_spModel);
        }
    }
    else if (const auto bundle = dynamic_cast<const Hedgehog::Mirage::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
            renderSky(it.get());
    }
    else if (const auto optimalBundle = dynamic_cast<const Hedgehog::Mirage::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
            renderSky(it);
    }
}

static FUNCTION_PTR(void, __cdecl, setSceneSurface, 0x64E960, void*, void*);

static void __cdecl implOfSceneRender(void* a1)
{
    setSceneSurface(a1, *reinterpret_cast<void**>(**reinterpret_cast<uint32_t**>(a1) + 84));

    const auto& renderScene = Sonic::CGameDocument::GetInstance()->GetWorld()->m_pMember->m_spRenderScene;

    const auto skyFindResult = renderScene->m_BundleMap.find("Sky");
    if (skyFindResult != renderScene->m_BundleMap.end())
        renderSky(skyFindResult->second.get());

    MaterialData::createPendingMaterials();

    const Hedgehog::Base::CStringSymbol symbols[] = { "Object", "Object_Overlay", "Player" };

    for (const auto& symbol : symbols)
    {
        const auto categoryFindResult = renderScene->m_BundleMap.find(symbol);
        if (categoryFindResult != renderScene->m_BundleMap.end())
            createInstancesAndBottomLevelAccelStructs(categoryFindResult->second.get(), true);
    }

    auto& traceRaysMessage = s_messageSender.makeMessage<MsgTraceRays>();

    traceRaysMessage.width = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 4);
    traceRaysMessage.height = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 6);

    traceRaysMessage.blueNoiseTextureId = reinterpret_cast<const Texture*>(
        Hedgehog::Mirage::CMirageDatabaseWrapper(Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spApplicationDatabase.get())
            .GetPictureData("blue_noise")->m_pD3DTexture)->getId();

    s_messageSender.endMessage();
}

void RaytracingRendering::init()
{
    WRITE_MEMORY(0x13DDFD8, uint32_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, uint32_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, uint32_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, uint32_t, 0); // Disable reflection map 2
    WRITE_MEMORY(0x13DDCA0, uint32_t, 1); // Override game scene child count
    WRITE_MEMORY(0x13DC2D8, void*, &implOfSceneRender); // Override scene render function
    WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
}
