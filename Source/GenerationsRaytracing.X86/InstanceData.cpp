#include "InstanceData.h"

#include "ModelData.h"
#include "Message.h"
#include "MessageSender.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"

class TerrainInstanceInfoDataEx : public Hedgehog::Mirage::CTerrainInstanceInfoData
{
public:
    uint32_t m_instanceId;
};

static std::unordered_set<TerrainInstanceInfoDataEx*> s_instancesToCreate;
static Mutex s_instanceCreateMutex;

static std::unordered_set<InstanceInfoEx*> s_trackedInstances;
static Mutex s_instanceTrackMutex;

HOOK(TerrainInstanceInfoDataEx*, __fastcall, TerrainInstanceInfoDataConstructor, 0x717350, TerrainInstanceInfoDataEx* This)
{
    const auto result = originalTerrainInstanceInfoDataConstructor(This);

    This->m_instanceId = NULL;

    return result;
}

HOOK(void, __fastcall, TerrainInstanceInfoDataDestructor, 0x717090, TerrainInstanceInfoDataEx* This)
{
    s_instanceCreateMutex.lock();
    s_instancesToCreate.erase(This);
    s_instanceCreateMutex.unlock();

    RaytracingUtil::releaseResource(RaytracingResourceType::Instance, This->m_instanceId);
    originalTerrainInstanceInfoDataDestructor(This);
}

static void __fastcall terrainInstanceInfoDataSetMadeOne(TerrainInstanceInfoDataEx* This)
{
    LockGuard lock(s_instanceCreateMutex);
    s_instancesToCreate.insert(This);

    This->SetMadeOne();
}

HOOK(InstanceInfoEx*, __fastcall, InstanceInfoConstructor, 0x7036A0, InstanceInfoEx* This)
{
    const auto result = originalInstanceInfoConstructor(This);

    This->m_instanceId = NULL;
    This->m_bottomLevelAccelStructId = NULL;
    new (std::addressof(This->m_poseVertexBuffer)) ComPtr<VertexBuffer>();
    This->m_headNodeIndex = 0;
    This->m_handledEyeMaterials = false;
    This->m_modelHash = 0;
    This->m_visibilityBits = 0;
    This->m_hashFrame = 0;

    return result;
}

HOOK(void, __fastcall, InstanceInfoDestructor, 0x7030B0, InstanceInfoEx* This)
{
    s_instanceTrackMutex.lock();
    s_trackedInstances.erase(This);
    s_instanceTrackMutex.unlock();

    RaytracingUtil::releaseResource(RaytracingResourceType::Instance, This->m_instanceId);
    RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, This->m_bottomLevelAccelStructId);

    This->m_poseVertexBuffer.~ComPtr();

    originalInstanceInfoDestructor(This);
}

void InstanceData::createPendingInstances()
{
    LockGuard lock(s_instanceCreateMutex);

    for (auto it = s_instancesToCreate.begin(); it != s_instancesToCreate.end();)
    {
        if ((*it)->IsMadeAll())
        {
            if ((*it)->m_spTerrainModel != nullptr)
            {
                const auto terrainModelEx =
                    reinterpret_cast<TerrainModelDataEx*>((*it)->m_spTerrainModel.get());

                ModelData::createBottomLevelAccelStruct(*terrainModelEx);

                if (terrainModelEx->m_bottomLevelAccelStructId != NULL)
                {
                    assert((*it)->m_instanceId == NULL);
                    (*it)->m_instanceId = s_idAllocator.allocate();

                    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

                    for (size_t i = 0; i < 3; i++)
                    {
                        for (size_t j = 0; j < 4; j++)
                            message.transform[i][j] = (*(*it)->m_scpTransform)(i, j);
                    }

                    message.instanceId = (*it)->m_instanceId;
                    message.bottomLevelAccelStructId = terrainModelEx->m_bottomLevelAccelStructId;
                    message.storePrevTransform = false;

                    s_messageSender.endMessage();
                }
            }
            it = s_instancesToCreate.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void InstanceData::trackInstance(InstanceInfoEx* instanceInfoEx)
{
    LockGuard lock(s_instanceTrackMutex);
    s_trackedInstances.emplace(instanceInfoEx);
}

void InstanceData::releaseUnusedInstances()
{
    LockGuard lock(s_instanceTrackMutex);

    for (auto it = s_trackedInstances.begin(); it != s_trackedInstances.end();)
    {
        if ((*it)->m_hashFrame != RaytracingRendering::s_frame)
        {
            RaytracingUtil::releaseResource(RaytracingResourceType::Instance, (*it)->m_instanceId);
            it = s_trackedInstances.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void InstanceData::init()
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
