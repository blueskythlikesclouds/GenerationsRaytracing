#include "TopLevelAccelStruct.h"

#include "BottomLevelAccelStruct.h"
#include "FreeListAllocator.h"
#include "Message.h"
#include "MessageSender.h"

static FreeListAllocator s_freeListAllocator;

class TerrainInstanceInfoDataEx : public Hedgehog::Mirage::CTerrainInstanceInfoData
{
public:
    uint32_t m_instanceId;
};

HOOK(TerrainInstanceInfoDataEx*, __fastcall, TerrainInstanceInfoDataConstructor, 0x717350, TerrainInstanceInfoDataEx* This)
{
    const auto result = originalTerrainInstanceInfoDataConstructor(This);

    This->m_instanceId = NULL;

    return result;
}

HOOK(void, __fastcall, TerrainInstanceInfoDataDestructor, 0x717090, TerrainInstanceInfoDataEx* This)
{
    if (This->m_instanceId != NULL)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();

        message.resourceType = MsgReleaseRaytracingResource::ResourceType::Instance;
        message.resourceId = This->m_instanceId;

        s_messageSender.endMessage();

        s_freeListAllocator.free(This->m_instanceId);
    }

    originalTerrainInstanceInfoDataDestructor(This);
}

static void __fastcall terrainInstanceInfoDataSetMadeOne(TerrainInstanceInfoDataEx* This)
{
    if (This->m_spTerrainModel != nullptr)
    {
        const auto terrainModelEx = 
            reinterpret_cast<TerrainModelDataEx*>(This->m_spTerrainModel.get());

        if (terrainModelEx->m_bottomLevelAccelStructId == NULL)
            terrainModelEx->m_bottomLevelAccelStructId = BottomLevelAccelStruct::allocate();

        auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 4; j++)
                message.transform[i][j] = (*This->m_scpTransform)(i, j);
        }

        if (This->m_instanceId == NULL)
            This->m_instanceId = s_freeListAllocator.allocate();

        message.instanceId = This->m_instanceId;
        message.bottomLevelAccelStructId = terrainModelEx->m_bottomLevelAccelStructId;
        message.storePrevTransform = false;

        s_messageSender.endMessage();
    }

    This->SetMadeOne();
}

HOOK(InstanceInfoEx*, __fastcall, InstanceInfoConstructor, 0x7036A0, InstanceInfoEx* This)
{
    const auto result = originalInstanceInfoConstructor(This);

    This->m_instanceId = NULL;
    This->m_bottomLevelAccelStructId = NULL;
    new (std::addressof(This->m_poseVertexBuffer)) ComPtr<VertexBuffer>();

    return result;
}

HOOK(void, __fastcall, InstanceInfoDestructor, 0x7030B0, InstanceInfoEx* This)
{
    if (This->m_instanceId != NULL)
    {
        auto& message = s_messageSender.makeMessage<MsgReleaseRaytracingResource>();
        message.resourceType = MsgReleaseRaytracingResource::ResourceType::Instance;
        message.resourceId = This->m_instanceId;
        s_messageSender.endMessage();

        s_freeListAllocator.free(This->m_instanceId);
    }

    if (This->m_bottomLevelAccelStructId != NULL)
        BottomLevelAccelStruct::release(This->m_bottomLevelAccelStructId);

    This->m_poseVertexBuffer.~ComPtr();

    originalInstanceInfoDestructor(This);
}

uint32_t TopLevelAccelStruct::allocate()
{
    return s_freeListAllocator.allocate();
}

void TopLevelAccelStruct::free(uint32_t id)
{
    s_freeListAllocator.free(id);
}

void TopLevelAccelStruct::init()
{
    WRITE_MEMORY(0x7176AC, uint32_t, sizeof(TerrainInstanceInfoDataEx));
    WRITE_MEMORY(0x725593, uint32_t, sizeof(TerrainInstanceInfoDataEx));
    WRITE_MEMORY(0X72FC7C, uint32_t, sizeof(TerrainInstanceInfoDataEx));

    INSTALL_HOOK(TerrainInstanceInfoDataConstructor);
    INSTALL_HOOK(TerrainInstanceInfoDataDestructor);

    WRITE_JUMP(0x73A382, terrainInstanceInfoDataSetMadeOne);
    WRITE_JUMP(0x738221, terrainInstanceInfoDataSetMadeOne);
    WRITE_JUMP(0x738891, terrainInstanceInfoDataSetMadeOne);
    WRITE_JUMP(0x738AA5, terrainInstanceInfoDataSetMadeOne);
    WRITE_JUMP(0x736990, terrainInstanceInfoDataSetMadeOne);
    WRITE_JUMP(0x738F6B, terrainInstanceInfoDataSetMadeOne);

    WRITE_MEMORY(0x701D4E, uint32_t, sizeof(InstanceInfoEx));
    WRITE_MEMORY(0x713089, uint32_t, sizeof(InstanceInfoEx));

    INSTALL_HOOK(InstanceInfoConstructor);
    INSTALL_HOOK(InstanceInfoDestructor);
}
