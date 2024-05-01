#include "InstanceData.h"

#include "ModelData.h"
#include "Message.h"
#include "MessageSender.h"
#include "PlayableParam.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"

static std::unordered_set<TerrainInstanceInfoDataEx*> s_instancesToCreate;
static std::unordered_multimap<uint32_t, TerrainInstanceInfoDataEx*> s_instanceSubsets;
static Mutex s_terrainInstanceMutex;

static std::unordered_set<InstanceInfoEx*> s_trackedInstances;
static Mutex s_instanceMutex;

HOOK(TerrainInstanceInfoDataEx*, __fastcall, TerrainInstanceInfoDataConstructor, 0x717350, TerrainInstanceInfoDataEx* This)
{
    const auto result = originalTerrainInstanceInfoDataConstructor(This);

    for (auto& instanceId : This->m_instanceIds)
        instanceId = NULL;

    new (&This->m_subsetIterator) decltype(This->m_subsetIterator) ();
    This->m_hasValidIterator = false;

    return result;
}

HOOK(void, __fastcall, TerrainInstanceInfoDataDestructor, 0x717090, TerrainInstanceInfoDataEx* This)
{
    LockGuard lock(s_terrainInstanceMutex);

    s_instancesToCreate.erase(This);
    if (This->m_hasValidIterator)
        s_instanceSubsets.erase(This->m_subsetIterator);

    PlayableParam::removeInstance(This);

    for (auto& instanceId : This->m_instanceIds)
        RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceId);

    originalTerrainInstanceInfoDataDestructor(This);
}

HOOK(InstanceInfoEx*, __fastcall, InstanceInfoConstructor, 0x7036A0, InstanceInfoEx* This)
{
    const auto result = originalInstanceInfoConstructor(This);

    for (auto& instanceId : This->m_instanceIds)
        instanceId = NULL;

    for (auto& bottomLevelAccelStructId : This->m_bottomLevelAccelStructIds)
        bottomLevelAccelStructId = NULL;

    new (std::addressof(This->m_poseVertexBuffer)) ComPtr<VertexBuffer>();
    This->m_headNodeIndex = 0;
    This->m_handledEyeMaterials = false;
    This->m_modelHash = 0;
    This->m_hashFrame = 0;
    This->m_chrPlayableMenuParam = 10000.0f;
    new (&This->m_effectMap) decltype(This->m_effectMap)();
    This->m_matrixHash = 0;
    This->m_prevMatrixHash = 0;

    return result;
}

HOOK(void, __fastcall, InstanceInfoDestructor, 0x7030B0, InstanceInfoEx* This)
{
    s_instanceMutex.lock();
    s_trackedInstances.erase(This);
    s_instanceMutex.unlock();

    This->m_effectMap.~unordered_map();
    This->m_poseVertexBuffer.~ComPtr();

    for (auto& bottomLevelAccelStructId : This->m_bottomLevelAccelStructIds)
        RaytracingUtil::releaseResource(RaytracingResourceType::BottomLevelAccelStruct, bottomLevelAccelStructId);

    for (auto& instanceId : This->m_instanceIds)
        RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceId);

    originalInstanceInfoDestructor(This);
}

void InstanceData::createPendingInstances(Hedgehog::Mirage::CRenderingDevice* renderingDevice)
{
    LockGuard lock(s_terrainInstanceMutex);

    for (auto it = s_instancesToCreate.begin(); it != s_instancesToCreate.end();)
    {
        if ((*it)->IsMadeAll())
        {
            if ((*it)->m_spTerrainModel != nullptr)
            {
                const auto terrainModelEx =
                    reinterpret_cast<TerrainModelDataEx*>((*it)->m_spTerrainModel.get());

                ModelData::createBottomLevelAccelStructs(*terrainModelEx);

                const bool isMirrored = (*it)->m_scpTransform->determinant() < 0.0f;

                for (size_t i = 0; i < _countof(s_instanceMasks); i++)
                {
                    const auto bottomLevelAccelStructId = terrainModelEx->m_bottomLevelAccelStructIds[i];

                    if (bottomLevelAccelStructId != NULL)
                    {
                        auto& instanceId = (*it)->m_instanceIds[i];
                        assert(instanceId == NULL);
                        instanceId = s_idAllocator.allocate();

                        auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);

                        for (size_t j = 0; j < 3; j++)
                        {
                            for (size_t k = 0; k < 4; k++)
                                message.transform[j][k] = (*(*it)->m_scpTransform)(j, k);
                        }

                        message.instanceId = instanceId;
                        message.bottomLevelAccelStructId = bottomLevelAccelStructId;
                        message.storePrevTransform = false;
                        message.isMirrored = isMirrored;
                        message.instanceMask = s_instanceMasks[i].instanceMask;
                        message.chrPlayableMenuParam = 10000.0f;

                        s_messageSender.endMessage();
                    }
                }
            }
            it = s_instancesToCreate.erase(it);
        }
        else
        {
            ++it;
        }
    }

    PlayableParam::updatePlayableParams(renderingDevice);
}

void InstanceData::trackInstance(InstanceInfoEx* instanceInfoEx)
{
    LockGuard lock(s_instanceMutex);
    s_trackedInstances.emplace(instanceInfoEx);
}

void InstanceData::releaseUnusedInstances()
{
    LockGuard lock(s_instanceMutex);

    for (auto it = s_trackedInstances.begin(); it != s_trackedInstances.end();)
    {
        if ((*it)->m_hashFrame != RaytracingRendering::s_frame)
        {
            for (auto& instanceId : (*it)->m_instanceIds)
                RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceId);

            it = s_trackedInstances.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

static uint32_t* __stdcall instanceSubsetMidAsmHook(uint32_t* status, TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx)
{
    LockGuard lock(s_terrainInstanceMutex);

    bool alreadyCreated = false;

    for (const auto& instanceId : terrainInstanceInfoDataEx->m_instanceIds)
        alreadyCreated |= (instanceId != NULL);

    if (!alreadyCreated)
    {
        if (status != reinterpret_cast<uint32_t*>(0x1B2449C))
        {
            if (terrainInstanceInfoDataEx->m_hasValidIterator)
                s_instanceSubsets.erase(terrainInstanceInfoDataEx->m_subsetIterator);
            else
                terrainInstanceInfoDataEx->m_hasValidIterator = true;

            terrainInstanceInfoDataEx->m_subsetIterator = s_instanceSubsets.emplace(*(status - 1), terrainInstanceInfoDataEx);

            if (*status != 1)
                s_instancesToCreate.emplace(terrainInstanceInfoDataEx);
        }
        else
        {
            s_instancesToCreate.emplace(terrainInstanceInfoDataEx);
        }

        PlayableParam::trackInstance(terrainInstanceInfoDataEx);
    }

    return status;
}

static uint32_t instanceSubsetTrampolineReturnAddr = 0x712709;

static void __declspec(naked) instanceSubsetTrampoline()
{
    __asm
    {
        push [edi]
        push eax
        call instanceSubsetMidAsmHook

        mov ebp, [esp + 0x30]
        mov [esi + 0x30], eax
        jmp [instanceSubsetTrampolineReturnAddr]
    }
}

static void showTerrainInstanceSubsets(uint32_t subsetId)
{
    LockGuard lock(s_terrainInstanceMutex);

    auto [begin, end] = s_instanceSubsets.equal_range(subsetId);
    for (auto it = begin; it != end; ++it)
    {
        bool alreadyCreated = false;

        for (const auto& instanceId : it->second->m_instanceIds)
            alreadyCreated |= (instanceId != NULL);

        if (!alreadyCreated)
            s_instancesToCreate.emplace(it->second);
    }
}

static void hideTerrainInstanceSubsets(uint32_t subsetId)
{
    LockGuard lock(s_terrainInstanceMutex);

    auto [begin, end] = s_instanceSubsets.equal_range(subsetId);
    for (auto it = begin; it != end; ++it)
    {
        for (auto& instanceId : it->second->m_instanceIds)
            RaytracingUtil::releaseResource(RaytracingResourceType::Instance, instanceId);
    }
}

HOOK(void, __fastcall, ProcMsgShowTerrainInstanceSubset, 0xD50870, Sonic::CTerrainManager2nd* This, void* _, uint32_t* message)
{
    showTerrainInstanceSubsets(*(message + 4));
    originalProcMsgShowTerrainInstanceSubset(This, _, message);
}

HOOK(void, __fastcall, ProcMsgHideTerrainInstanceSubset, 0xD50F90, Sonic::CTerrainManager2nd* This, void* _, uint32_t* message)
{
    hideTerrainInstanceSubsets(*(message + 4));
    originalProcMsgHideTerrainInstanceSubset(This, _, message);
}

HOOK(void, __fastcall, ProcMsgChangeTerrainInstanceSubsetVisibilityState, 0xD50F20, Sonic::CTerrainManager2nd* This, void* _, uint32_t* message)
{
    if (*(message + 5) == 1)
        hideTerrainInstanceSubsets(*(message + 4));
    else
        showTerrainInstanceSubsets(*(message + 4));

    originalProcMsgChangeTerrainInstanceSubsetVisibilityState(This, _, message);
}

HOOK(void, __fastcall, ProcMsgRestartStage, 0xD50E30, Sonic::CTerrainManager2nd* This, void* _, uint32_t* message)
{
    originalProcMsgRestartStage(This, _, message);

    for (auto& [subsetId, subset] : This->m_pTerrainInitializeInfo->TerrainGroupSubsetVisibilityStates)
    {
        if (subset->m_CurrentState == 1)
            hideTerrainInstanceSubsets(subsetId);
        else
            showTerrainInstanceSubsets(subsetId);
    }
}

void InstanceData::init()
{
    WRITE_MEMORY(0x7176AC, uint32_t, sizeof(TerrainInstanceInfoDataEx));
    WRITE_MEMORY(0x725593, uint32_t, sizeof(TerrainInstanceInfoDataEx));
    WRITE_MEMORY(0X72FC7C, uint32_t, sizeof(TerrainInstanceInfoDataEx));

    INSTALL_HOOK(TerrainInstanceInfoDataConstructor);
    INSTALL_HOOK(TerrainInstanceInfoDataDestructor);

    WRITE_MEMORY(0x701D4E, uint32_t, sizeof(InstanceInfoEx));
    WRITE_MEMORY(0x713089, uint32_t, sizeof(InstanceInfoEx));

    INSTALL_HOOK(InstanceInfoConstructor);
    INSTALL_HOOK(InstanceInfoDestructor);

    WRITE_JUMP(0x712702, instanceSubsetTrampoline);
    INSTALL_HOOK(ProcMsgShowTerrainInstanceSubset);
    INSTALL_HOOK(ProcMsgHideTerrainInstanceSubset);
    INSTALL_HOOK(ProcMsgChangeTerrainInstanceSubsetVisibilityState);
    INSTALL_HOOK(ProcMsgRestartStage);
}
