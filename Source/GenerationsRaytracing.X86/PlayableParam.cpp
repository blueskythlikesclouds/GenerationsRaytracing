#include "PlayableParam.h"

#include "InstanceData.h"
#include "Message.h"
#include "MessageSender.h"

static std::unordered_map<TerrainInstanceInfoDataEx*, uint32_t> s_instances;

static uint32_t getPlayableParamId(Hedgehog::Base::CSharedString* name)
{
    uint32_t funcAddr = 0xD3FA30;
    volatile uint32_t result;
    __asm
    {
        mov eax, name
        call [funcAddr]
        mov result, eax
    }
    return result;
}

static float getPlayableParam(uint32_t id)
{
    uint32_t funcAddr = 0xD0A510;
    volatile float result;
    __asm
    {
        mov eax, id
        call [funcAddr]
        movss result, xmm0
    }
    return result;
}

void PlayableParam::trackInstance(TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx)
{
    const uint32_t id = getPlayableParamId(&terrainInstanceInfoDataEx->m_Name);
    if (id != 1000000)
        s_instances.emplace(terrainInstanceInfoDataEx, id);
}

void PlayableParam::removeInstance(TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx)
{
    s_instances.erase(terrainInstanceInfoDataEx);
}

void PlayableParam::updatePlayableParams(Hedgehog::Mirage::CRenderingDevice* renderingDevice)
{
    if (s_instances.empty() || strcmp(reinterpret_cast<const char*>(0x1E774D4), "pam000") != 0)
        return;

    auto& message = s_messageSender.makeMessage<MsgUpdatePlayableParam>(
        sizeof(MsgUpdatePlayableParam::InstanceData) * s_instances.size() * _countof(s_instanceMasks));

    auto instanceData = reinterpret_cast<MsgUpdatePlayableParam::InstanceData*>(message.data);

    for (const auto& instance : s_instances)
    {
        float playableParam0;
        float playableParam1;

        if (instance.second == *reinterpret_cast<uint32_t*>(0x1E5E408))
        {
            playableParam0 = 0.0f;
            playableParam1 = 1.0f;
        }
        else
        {
            playableParam0 = getPlayableParam(instance.second);
            playableParam1 = 0.0f;
        }

        float playableParam;

        if (playableParam1 >= 0.5f)
            playableParam = 1.0f - renderingDevice->m_RenderStateInfo.PlayableParamRestoreHeight - 0.0078125f;

        else if (playableParam0 <= 0.5f)
            playableParam = 10000.0f;

        else
            playableParam = -10001.0f;

        for (const auto& instanceId : instance.first->m_instanceIds)
        {
            instanceData->instanceId = instanceId;
            instanceData->playableParam = playableParam;
            ++instanceData;
        }
    }

    s_messageSender.endMessage();
}

