#include "RaytracingRendering.h"

#include "Configuration.h"
#include "ModelData.h"
#include "Device.h"
#include "MaterialData.h"
#include "Message.h"
#include "MessageSender.h"
#include "Texture.h"
#include "InstanceData.h"
#include "RaytracingParams.h"
#include "ToneMap.h"

static void createInstancesAndBottomLevelAccelStructs(Hedgehog::Mirage::CRenderable* renderable)
{
    if (!renderable->m_Enabled)
        return;

    if (auto element = dynamic_cast<Hedgehog::Mirage::CSingleElement*>(renderable))
    {
        if (element->m_spModel != nullptr && element->m_spModel->IsMadeOne() &&
            (element->m_spInstanceInfo->m_Flags & Hedgehog::Mirage::eInstanceInfoFlags_Invisible) == 0)
        {
            auto modelDataEx = reinterpret_cast<ModelDataEx*>(element->m_spModel.get());
            if (modelDataEx->m_noAoModel != nullptr && RaytracingParams::s_enableNoAoModels)
                modelDataEx = reinterpret_cast<ModelDataEx*>(modelDataEx->m_noAoModel.get());

            if (modelDataEx->IsMadeAll())
            {
                const auto instanceInfoEx = reinterpret_cast<InstanceInfoEx*>(element->m_spInstanceInfo.get());

                ModelData::processEyeMaterials(
                    *modelDataEx,
                    *instanceInfoEx,
                    element->m_MaterialMap);

                if (RaytracingParams::s_enable)
                {
                    if (element->m_spSingleElementEffect != nullptr)
                    {
                        ModelData::processSingleElementEffect(
                            element->m_MaterialMap,
                            element->m_spSingleElementEffect.get());
                    }
                }
                else
                {
                    for (auto it = element->m_MaterialMap.begin(); it != element->m_MaterialMap.end();)
                    {
                        if (it->second->m_TypeAndName == "RaytracingMaterialClone")
                            it = element->m_MaterialMap.erase(it);
                        else
                            ++it;
                    }

                    instanceInfoEx->m_handledEyeMaterials = false;
                }

                ModelData::createBottomLevelAccelStructs(
                    *modelDataEx,
                    *instanceInfoEx,
                    element->m_MaterialMap);

                InstanceData::trackInstance(instanceInfoEx);
            }

        }
    }
    else if (const auto bundle = dynamic_cast<const Hedgehog::Mirage::CBundle*>(renderable))
    {
        for (const auto& it : bundle->m_RenderableList)
            createInstancesAndBottomLevelAccelStructs(it.get());
    }
    else if (const auto optimalBundle = dynamic_cast<const Hedgehog::Mirage::COptimalBundle*>(renderable))
    {
        for (const auto it : optimalBundle->m_RenderableList)
            createInstancesAndBottomLevelAccelStructs(it);
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

static void createLocalLight(Hedgehog::Mirage::CLightData& lightData, size_t localLightId)
{
    auto& message = s_messageSender.makeMessage<MsgCreateLocalLight>();
    
    message.localLightId = localLightId;
    memcpy(message.position, lightData.m_Position.data(), sizeof(message.position));
    message.inRange = lightData.m_Range.z();
    memcpy(message.color, lightData.m_Color.data(), sizeof(message.color));
    message.outRange = lightData.m_Range.w();
    
    s_messageSender.endMessage();
}

// 2 elements for post processing on particles
static Hedgehog::FxRenderFramework::SDrawInstanceParam s_drawInstanceParams[2u];
static uint32_t s_drawInstanceParamCount;

static bool s_prevRaytracingEnable;

// Store for Better FxPipeline compatibility
static std::unique_ptr<Hedgehog::FxRenderFramework::SDrawInstanceParam[]> s_gameSceneChildren;
static uint32_t s_gameSceneChildCount;

static void initRaytracingPatches(bool enable)
{
    if (s_prevRaytracingEnable == enable)
        return;

    if (enable)
    {
        WRITE_MEMORY(0x13DDFD8, uint32_t, 0); // Disable shadow map

        WRITE_MEMORY(0x13DDBA0, uint32_t, 0); // Disable reflection map 1
        WRITE_MEMORY(0x13DDC20, uint32_t, 0); // Disable reflection map 2

        WRITE_MEMORY(0x13DDC9C, void*, s_drawInstanceParams); // Override game scene children
        WRITE_MEMORY(0x13DDCA0, uint32_t, s_drawInstanceParamCount); // Override game scene child count

        WRITE_MEMORY(0x72ACD0, uint8_t, 0xC2, 0x08, 0x00); // Disable GI texture
        WRITE_MEMORY(0x72E5C0, uint8_t, 0xC2, 0x08, 0x00); // Disable culling
    }
    else
    {
        WRITE_MEMORY(0x13DDFD8, uint32_t, 4); // Enable shadow map

        WRITE_MEMORY(0x13DDBA0, uint32_t, 22); // Enable reflection map 1
        WRITE_MEMORY(0x13DDC20, uint32_t, 22); // Enable reflection map 2

        WRITE_MEMORY(0x13DDC9C, void*, s_gameSceneChildren.get()); // Restore game scene children
        WRITE_MEMORY(0x13DDCA0, uint32_t, s_gameSceneChildCount); // Restore game scene child count

        WRITE_MEMORY(0x72ACD0, uint8_t, 0x53, 0x56, 0x57); // Enable GI texture
        WRITE_MEMORY(0x72E5C0, uint8_t, 0x56, 0x8B, 0xF1); // Enable culling
    }

    s_prevRaytracingEnable = enable;
}

static FUNCTION_PTR(void, __cdecl, sceneRender, 0x652110, void*);
static FUNCTION_PTR(void, __cdecl, setSceneSurface, 0x64E960, void*, void*);

static boost::shared_ptr<Hedgehog::Mirage::CModelData> s_curSky;
static float s_curBackGroundScale;

static Hedgehog::Mirage::CLightListData* s_curLightList;
static size_t s_localLightCount;
static uint32_t s_prevDebugView;
static uint32_t s_prevEnvMode;
static Hedgehog::Math::CVector s_prevSkyColor;
static Hedgehog::Math::CVector s_prevGroundColor;

// Deadlock workaround: Acquiring a handle to GameDocument hangs the loading
// thread when rendering, so let's call our logic only in the main thread.
static DWORD s_mainThreadId;

static void __cdecl implOfSceneRender(void* a1)
{
    bool resetAccumulation = RaytracingParams::update();
    const bool shouldRender = s_prevRaytracingEnable;
    initRaytracingPatches(RaytracingParams::s_enable);

    if (shouldRender && GetCurrentThreadId() == s_mainThreadId)
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
        const float prevBackGroundScale = s_curBackGroundScale;

        if (RaytracingParams::s_sky != nullptr && RaytracingParams::s_sky->IsMadeAll())
        {
            s_curSky = RaytracingParams::s_sky;
        }
        else
        {
            const auto skyFindResult = renderScene->m_BundleMap.find("Sky");
            if (skyFindResult != renderScene->m_BundleMap.end())
                s_curSky = findSky(skyFindResult->second.get());
            else
                s_curSky = nullptr;
        }

        s_curBackGroundScale = *reinterpret_cast<const float*>(0x1A489EC);

        if (s_curSky != prevSky || s_curBackGroundScale != prevBackGroundScale)
        {
            if (s_curSky != nullptr)
                ModelData::renderSky(*s_curSky);

            resetAccumulation = true;
        }

        MaterialData::createPendingMaterials();
        InstanceData::createPendingInstances();

        const Hedgehog::Base::CStringSymbol symbols[] = { "Object", "Object_Overlay", "Player" };

        for (const auto& symbol : symbols)
        {
            const auto categoryFindResult = renderScene->m_BundleMap.find(symbol);
            if (categoryFindResult != renderScene->m_BundleMap.end())
                createInstancesAndBottomLevelAccelStructs(categoryFindResult->second.get());
        }

        InstanceData::releaseUnusedInstances();

        size_t localLightCount = 0;

        if (const auto gameDocument = Sonic::CGameDocument::GetInstance())
        {
            const auto& lightManager = gameDocument->m_pMember->m_spLightManager;

            if (lightManager != nullptr)
            {
                if (lightManager->m_pStaticLightContext && lightManager->m_pStaticLightContext->m_spLightListData && 
                    lightManager->m_pStaticLightContext->m_spLightListData->IsMadeAll())
                {
                    const auto& lightListData = lightManager->m_pStaticLightContext->m_spLightListData;

                    if (s_curLightList != lightListData.get())
                    {
                        s_curLightList = lightListData.get();
                        s_localLightCount = 0;

                        for (const auto& lightData : lightListData->m_Lights)
                        {
                            if (lightData->IsMadeAll() && lightData->m_Type == Hedgehog::Mirage::eLightType_Point)
                            {
                                createLocalLight(*lightData, s_localLightCount);
                                ++s_localLightCount;
                            }
                        }

                        resetAccumulation = true;
                    }
                }

                localLightCount = s_localLightCount;

                if (lightManager->m_pLocalLightContext != nullptr)
                {
                    for (auto& localLight : lightManager->m_pLocalLightContext->m_LocalLights)
                    {
                        createLocalLight(*localLight->m_spLight, localLightCount);
                        ++localLightCount;
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
        traceRaysMessage.localLightCount = localLightCount;

        traceRaysMessage.diffusePower = RaytracingParams::s_diffusePower;
        traceRaysMessage.lightPower = RaytracingParams::s_lightPower;
        traceRaysMessage.emissivePower = RaytracingParams::s_emissivePower;
        traceRaysMessage.skyPower = RaytracingParams::s_skyPower;
        traceRaysMessage.debugView = RaytracingParams::s_debugView;
        traceRaysMessage.envMode = RaytracingParams::s_envMode;
        memcpy(traceRaysMessage.skyColor, RaytracingParams::s_skyColor.data(), sizeof(traceRaysMessage.skyColor));

        if (RaytracingParams::s_groundColor.squaredNorm() > 0.0001)
            memcpy(traceRaysMessage.groundColor, RaytracingParams::s_groundColor.data(), sizeof(traceRaysMessage.groundColor));
        else
            memcpy(traceRaysMessage.groundColor, RaytracingParams::s_skyColor.data(), sizeof(traceRaysMessage.groundColor));

        traceRaysMessage.useSkyTexture = s_curSky != nullptr;

        if (const auto gameDocument = Sonic::CGameDocument::GetInstance())
        {
            const auto renderDirector = gameDocument->m_pMember->m_spRenderDirector.get();
            const auto mtfxInternal = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(renderDirector) + 0xC4);
            const auto bgColor = reinterpret_cast<uint8_t*>(mtfxInternal + 0x1A0);

            traceRaysMessage.backgroundColor[0] = static_cast<float>(bgColor[2]) / 255.0f;
            traceRaysMessage.backgroundColor[1] = static_cast<float>(bgColor[1]) / 255.0f;
            traceRaysMessage.backgroundColor[2] = static_cast<float>(bgColor[0]) / 255.0f;
        }
        else
        {
            memset(traceRaysMessage.backgroundColor, 0, sizeof(traceRaysMessage.backgroundColor));
        }

        traceRaysMessage.upscaler = RaytracingParams::s_upscaler == 0 ? Configuration::s_upscaler + 1 : RaytracingParams::s_upscaler;
        traceRaysMessage.qualityMode = RaytracingParams::s_qualityMode == 0 ? Configuration::s_qualityMode + 1 : RaytracingParams::s_qualityMode;

        traceRaysMessage.adaptionLuminanceTextureId = (*reinterpret_cast<Texture**>(
            *reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(a1) + 20) + 0x658))->getId();

        traceRaysMessage.middleGray = *reinterpret_cast<float*>(0x1A572D0);

        s_messageSender.endMessage();

        ++RaytracingRendering::s_frame;
    }
    else
    {
        sceneRender(a1);
    }
}

void RaytracingRendering::init()
{
    s_mainThreadId = GetCurrentThreadId();

    WRITE_MEMORY(0x13DDB20, uint32_t, 0); // Disable sky render
}

void RaytracingRendering::postInit()
{
    s_gameSceneChildCount = *reinterpret_cast<uint32_t*>(0x13DDCA0);
    s_gameSceneChildren = std::make_unique<Hedgehog::FxRenderFramework::SDrawInstanceParam[]>(s_gameSceneChildCount);

    memcpy(s_gameSceneChildren.get(), *reinterpret_cast<void**>(0x13DDC9C), 
        s_gameSceneChildCount * sizeof(Hedgehog::FxRenderFramework::SDrawInstanceParam));

    s_gameSceneChildren[0].pCallback = implOfSceneRender;

    WRITE_MEMORY(0x13DDC9C, void*, s_gameSceneChildren.get());

    // Init draw instance params
    memcpy(s_drawInstanceParams, reinterpret_cast<void*>(0x13DC2C8), 
        sizeof(Hedgehog::FxRenderFramework::SDrawInstanceParam));

    s_drawInstanceParams[0].pCallback = implOfSceneRender;

    if (*reinterpret_cast<uint32_t*>(0x13DD790) == 2)
    {
        memcpy(s_drawInstanceParams + 1, reinterpret_cast<void*>(0x13DC8C8), 
            sizeof(Hedgehog::FxRenderFramework::SDrawInstanceParam));

        s_drawInstanceParamCount = 2;
    }
    else
    {
        s_drawInstanceParamCount = 1;
    }
}
