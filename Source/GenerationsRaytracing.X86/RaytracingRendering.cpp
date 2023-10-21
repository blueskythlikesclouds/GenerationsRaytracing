#include "RaytracingRendering.h"

#include "ModelData.h"
#include "Device.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"
#include "InstanceData.h"
#include "RaytracingParams.h"

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

static boost::shared_ptr<Hedgehog::Mirage::CModelData> findSky(Hedgehog::Mirage::CRenderable* renderable)
{
    if (!renderable->m_Enabled)
        return nullptr;

    if (auto element = dynamic_cast<const Hedgehog::Mirage::CSingleElement*>(renderable))
    {
        if (element->m_spModel->IsMadeAll() && 
            (element->m_spInstanceInfo->m_Flags & Hedgehog::Mirage::eInstanceInfoFlags_Invisible) == 0)
        {
            return element->m_spModel;
        }
    }
    else if (const auto bundle = dynamic_cast<const Hedgehog::Mirage::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
        {
            const auto model = findSky(it.get());
            if (model != nullptr)
                return model;
        }
    }
    else if (const auto optimalBundle = dynamic_cast<const Hedgehog::Mirage::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
        {
            const auto model = findSky(it);
            if (model != nullptr)
                return model;
        }
    }

    return nullptr;
}

static bool s_prevRaytracingEnable = false;

static void initRaytracingPatches(bool enable)
{
    if (s_prevRaytracingEnable == enable)
        return;

    if (enable)
    {
        WRITE_MEMORY(0x13DDFD8, uint32_t, 0); // Disable shadow map
        WRITE_MEMORY(0x13DDBA0, uint32_t, 0); // Disable reflection map 1
        WRITE_MEMORY(0x13DDC20, uint32_t, 0); // Disable reflection map 2
        WRITE_MEMORY(0x13DDCA0, uint32_t, 1); // Override game scene child count
        WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
    }
    else
    {
        WRITE_MEMORY(0x13DDFD8, size_t, 4); // Enable shadow map
        WRITE_MEMORY(0x13DDBA0, size_t, 22); // Enable reflection map 1
        WRITE_MEMORY(0x13DDC20, size_t, 22); // Enable reflection map 2
        WRITE_MEMORY(0x13DDCA0, size_t, 22); // Restore game scene child count
        WRITE_MEMORY(0x72ACD0, uint8_t, 0x53, 0x56, 0x57); // Enable GI texture
    }

    s_prevRaytracingEnable = enable;
}

static FUNCTION_PTR(void, __cdecl, sceneRender, 0x652110, void*);
static FUNCTION_PTR(void, __cdecl, setSceneSurface, 0x64E960, void*, void*);

static boost::shared_ptr<Hedgehog::Mirage::CModelData> s_curSky;

static Hedgehog::Mirage::CLightListData* s_curLightList;
static size_t s_localLightCount;
static uint32_t s_prevDebugView;
static uint32_t s_prevEnvMode;
static Hedgehog::Math::CVector s_prevSkyColor;
static Hedgehog::Math::CVector s_prevGroundColor;

static void __cdecl implOfSceneRender(void* a1)
{
    bool resetAccumulation = RaytracingParams::update();
    const bool shouldRender = s_prevRaytracingEnable;
    initRaytracingPatches(RaytracingParams::s_enable);

    if (shouldRender)
    {
        setSceneSurface(a1, *reinterpret_cast<void**>(**reinterpret_cast<uint32_t**>(a1) + 84));

        const auto renderingDevice = **reinterpret_cast<hh::mr::CRenderingDevice***>(reinterpret_cast<uintptr_t>(a1) + 16);
        const auto& renderScene = Sonic::CGameDocument::GetInstance()->GetWorld()->m_pMember->m_spRenderScene;

        if (RaytracingParams::s_light != nullptr && RaytracingParams::s_light->IsMadeAll())
        {
            renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(183, RaytracingParams::s_light->m_Direction.data(), 1);
            renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(184, RaytracingParams::s_light->m_Color.data(), 1);
            renderingDevice->m_pD3DDevice->SetVertexShaderConstantF(185, RaytracingParams::s_light->m_Color.data(), 1);

            renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(10, RaytracingParams::s_light->m_Direction.data(), 1);
            renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(36, RaytracingParams::s_light->m_Color.data(), 1);
            renderingDevice->m_pD3DDevice->SetPixelShaderConstantF(37, RaytracingParams::s_light->m_Color.data(), 1);
        }

        const auto prevSky = s_curSky;

        if (RaytracingParams::s_sky != nullptr && RaytracingParams::s_sky->IsMadeAll())
        {
            s_curSky = RaytracingParams::s_sky;
        }
        else
        {
            const auto skyFindResult = renderScene->m_BundleMap.find("Sky");
            if (skyFindResult != renderScene->m_BundleMap.end())
                s_curSky = findSky(skyFindResult->second.get());
        }

        if (s_curSky != prevSky)
            ModelData::renderSky(*s_curSky);

        InstanceData::createPendingInstances();
        MaterialData::createPendingMaterials();

        const Hedgehog::Base::CStringSymbol symbols[] = { "Object", "Object_Overlay", "Player" };

        for (const auto& symbol : symbols)
        {
            const auto categoryFindResult = renderScene->m_BundleMap.find(symbol);
            if (categoryFindResult != renderScene->m_BundleMap.end())
                createInstancesAndBottomLevelAccelStructs(categoryFindResult->second.get(), true);
        }

        if (const auto gameDocument = Sonic::CGameDocument::GetInstance())
        {
            if (gameDocument->m_pMember->m_spLightManager &&
                gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext &&
                gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext->m_spLightListData &&
                gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext->m_spLightListData->IsMadeAll())
            {
                if (s_curLightList != gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext->m_spLightListData.get())
                {
                    s_curLightList = gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext->m_spLightListData.get();
                    s_localLightCount = 0;

                    for (const auto& lightData : gameDocument->m_pMember->m_spLightManager->m_pStaticLightContext->m_spLightListData->m_Lights)
                    {
                        if (lightData->IsMadeAll() && lightData->m_Type == Hedgehog::Mirage::eLightType_Point)
                        {
                            auto& message = s_messageSender.makeMessage<MsgCreateLocalLight>();

                            message.localLightId = s_localLightCount;
                            memcpy(message.position, lightData->m_Position.data(), sizeof(message.position));
                            message.inRange = lightData->m_Range.z();
                            memcpy(message.color, lightData->m_Color.data(), sizeof(message.color));
                            message.outRange = lightData->m_Range.w();

                            s_messageSender.endMessage();

                            ++s_localLightCount;
                        }
                    }
                }
            }
        }

        if (s_prevDebugView != RaytracingParams::s_debugView ||
            s_prevEnvMode != RaytracingParams::s_envMode ||
            s_prevSkyColor != RaytracingParams::s_skyColor ||
            s_prevGroundColor != RaytracingParams::s_groundColor)
        {
            resetAccumulation = true;
        }

        s_prevDebugView = RaytracingParams::s_debugView;
        s_prevEnvMode = RaytracingParams::s_envMode;
        s_prevSkyColor = RaytracingParams::s_skyColor;
        s_prevGroundColor = RaytracingParams::s_groundColor;

        auto& traceRaysMessage = s_messageSender.makeMessage<MsgTraceRays>();

        traceRaysMessage.width = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 4);
        traceRaysMessage.height = *reinterpret_cast<uint16_t*>(**static_cast<uintptr_t**>(a1) + 6);

        traceRaysMessage.blueNoiseTextureId = reinterpret_cast<const Texture*>(
            Hedgehog::Mirage::CMirageDatabaseWrapper(Sonic::CApplicationDocument::GetInstance()->m_pMember->m_spApplicationDatabase.get())
            .GetPictureData("blue_noise")->m_pD3DTexture)->getId();

        traceRaysMessage.resetAccumulation = resetAccumulation;
        traceRaysMessage.localLightCount = s_localLightCount;

        traceRaysMessage.diffusePower = RaytracingParams::s_diffusePower;
        traceRaysMessage.lightPower = RaytracingParams::s_lightPower;
        traceRaysMessage.emissivePower = RaytracingParams::s_emissivePower;
        traceRaysMessage.debugView = RaytracingParams::s_debugView;
        traceRaysMessage.envMode = RaytracingParams::s_envMode;
        memcpy(traceRaysMessage.skyColor, RaytracingParams::s_skyColor.data(), sizeof(traceRaysMessage.skyColor));

        if (RaytracingParams::s_groundColor.squaredNorm() > 0.0001)
            memcpy(traceRaysMessage.groundColor, RaytracingParams::s_groundColor.data(), sizeof(traceRaysMessage.groundColor));
        else
            memcpy(traceRaysMessage.groundColor, RaytracingParams::s_skyColor.data(), sizeof(traceRaysMessage.groundColor));

        s_messageSender.endMessage();
    }
    else
    {
        sceneRender(a1);
    }
}

void RaytracingRendering::init()
{
    WRITE_MEMORY(0x13DDB20, uint32_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DC2D8, void*, &implOfSceneRender); // Override scene render function
}
