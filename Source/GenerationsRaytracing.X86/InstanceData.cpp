#include "InstanceData.h"

#include "ModelData.h"
#include "Message.h"
#include "MessageSender.h"
#include "PlayableParam.h"
#include "RaytracingRendering.h"
#include "RaytracingUtil.h"

static std::unordered_set<TerrainInstanceInfoDataEx*> s_instances;
static std::unordered_multimap<uint32_t, TerrainInstanceInfoDataEx*> s_instanceSubsets;
static Mutex s_terrainInstanceMutex;

static std::unordered_set<InstanceInfoEx*> s_trackedInstances;
static Mutex s_instanceMutex;

HOOK(TerrainInstanceInfoDataEx*, __fastcall, TerrainInstanceInfoDataConstructor, 0x717350, TerrainInstanceInfoDataEx* This)
{
    const auto result = originalTerrainInstanceInfoDataConstructor(This);

    for (auto& instanceFrame : This->m_instanceFrames)
        instanceFrame = 0;

    new (&This->m_subsetIterator) decltype(This->m_subsetIterator) ();
    This->m_hasValidIterator = false;

    return result;
}

HOOK(void, __fastcall, TerrainInstanceInfoDataDestructor, 0x717090, TerrainInstanceInfoDataEx* This)
{
    LockGuard lock(s_terrainInstanceMutex);

    s_instances.erase(This);
    if (This->m_hasValidIterator)
        s_instanceSubsets.erase(This->m_subsetIterator);

    originalTerrainInstanceInfoDataDestructor(This);
}

HOOK(InstanceInfoEx*, __fastcall, InstanceInfoConstructor, 0x7036A0, InstanceInfoEx* This)
{
    const auto result = originalInstanceInfoConstructor(This);

    for (auto& instanceFrame : This->m_instanceFrames)
        instanceFrame = 0;

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
    This->m_enableForceAlphaColor = false;
    This->m_forceAlphaColor = 1.0f;

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

    originalInstanceInfoDestructor(This);
}

void InstanceData::createInstances(Hedgehog::Mirage::CRenderingDevice* renderingDevice)
{
    LockGuard lock(s_terrainInstanceMutex);

    for (auto& instance : s_instances)
    {
        if (instance->IsMadeAll() && instance->m_spTerrainModel != nullptr)
        {
            const auto terrainModelEx =
                reinterpret_cast<TerrainModelDataEx*>(instance->m_spTerrainModel.get());

            ModelData::createBottomLevelAccelStructs(*terrainModelEx);

            float transform[3][4];

            for (size_t i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 4; j++)
                {
                    transform[i][j] = (*instance->m_scpTransform)(i, j);

                    if (j == 3)
                        transform[i][j] += RaytracingRendering::s_worldShift[i];
                }
            }

            const bool isMirrored = instance->m_scpTransform->determinant() < 0.0f;
            const float playableParam = PlayableParam::getPlayableParam(instance, renderingDevice);

            for (size_t i = 0; i < _countof(s_instanceMasks); i++)
            {
                const auto bottomLevelAccelStructId = terrainModelEx->m_bottomLevelAccelStructIds[i];

                if (bottomLevelAccelStructId != NULL && terrainModelEx->m_buildFrames[i] != RaytracingRendering::s_frame)
                {
                    const bool shouldCopyPrevTransform = (instance->m_instanceFrames[i] + 1) == RaytracingRendering::s_frame;
                    instance->m_instanceFrames[i] = RaytracingRendering::s_frame;

                    auto& message = s_messageSender.makeMessage<MsgCreateInstance>(0);
                    memcpy(message.transform, transform, sizeof(message.transform));
                    memcpy(message.prevTransform, shouldCopyPrevTransform ? instance->m_prevTransform : transform, sizeof(message.prevTransform));
                    message.bottomLevelAccelStructId = bottomLevelAccelStructId;
                    message.isMirrored = isMirrored;
                    message.instanceMask = s_instanceMasks[i].instanceMask;
                    message.playableParam = playableParam;
                    message.chrPlayableMenuParam = 10000.0f;
                    message.forceAlphaColor = 1.0f;
                    s_messageSender.endMessage();
                }
            }

            memcpy(instance->m_prevTransform, transform, sizeof(instance->m_prevTransform));
        }
    }
}

static uint32_t* __stdcall instanceSubsetMidAsmHook(uint32_t* status, TerrainInstanceInfoDataEx* terrainInstanceInfoDataEx)
{
    LockGuard lock(s_terrainInstanceMutex);

    if (status != reinterpret_cast<uint32_t*>(0x1B2449C))
    {
        if (terrainInstanceInfoDataEx->m_hasValidIterator)
            s_instanceSubsets.erase(terrainInstanceInfoDataEx->m_subsetIterator);
        else
            terrainInstanceInfoDataEx->m_hasValidIterator = true;
    
        terrainInstanceInfoDataEx->m_subsetIterator = s_instanceSubsets.emplace(*(status - 1), terrainInstanceInfoDataEx);
    
        if (*status != 1)
            s_instances.emplace(terrainInstanceInfoDataEx);
        else
            s_instances.erase(terrainInstanceInfoDataEx);
    }
    else
    {
        s_instances.emplace(terrainInstanceInfoDataEx);
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
        s_instances.emplace(it->second);
}

static void hideTerrainInstanceSubsets(uint32_t subsetId)
{
    LockGuard lock(s_terrainInstanceMutex);

    auto [begin, end] = s_instanceSubsets.equal_range(subsetId);
    for (auto it = begin; it != end; ++it)
        s_instances.erase(it->second);
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
