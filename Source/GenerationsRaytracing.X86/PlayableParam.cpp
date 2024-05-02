#include "PlayableParam.h"

#include "InstanceData.h"
#include "Message.h"
#include "MessageSender.h"

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

float PlayableParam::getPlayableParam(TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx, Hedgehog::Mirage::CRenderingDevice* renderingDevice)
{
    if (strcmp(reinterpret_cast<const char*>(0x1E774D4), "pam000") == 0)
    {
        const uint32_t id = getPlayableParamId(&terrainInstanceInfoDataEx->m_Name);
        if (id != 1000000)
        {
            float playableParam0;
            float playableParam1;

            if (id == *reinterpret_cast<uint32_t*>(0x1E5E408))
            {
                playableParam0 = 0.0f;
                playableParam1 = 1.0f;
            }
            else
            {
                playableParam0 = ::getPlayableParam(id);
                playableParam1 = 0.0f;
            }

            if (playableParam1 >= 0.5f)
                return 1.0f - renderingDevice->m_RenderStateInfo.PlayableParamRestoreHeight - 0.0078125f;

            else if (playableParam0 <= 0.5f)
                return 10000.0f;
        }
    }

    return -10001.0f;
}

