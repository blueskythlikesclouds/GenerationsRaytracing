#pragma once

#include "FreeListAllocator.h"
#include "InstanceMask.h"

class InstanceInfoEx;

class TerrainModelDataEx : public Hedgehog::Mirage::CTerrainModelData
{
public:
    uint32_t m_bottomLevelAccelStructIds[_countof(s_instanceMasks)];
    uint64_t m_bottomLevelAccelStructFrames[_countof(s_instanceMasks)];
};

class ModelDataEx : public Hedgehog::Mirage::CModelData
{
public:
    uint32_t m_bottomLevelAccelStructIds[_countof(s_instanceMasks)];
    XXH32_hash_t m_modelHash;
    uint64_t m_hashFrame;
    bool m_enableSkinning;
    boost::shared_ptr<CModelData> m_noAoModel;
};

using MaterialMap = hh::map<Hedgehog::Mirage::CMaterialData*, boost::shared_ptr<Hedgehog::Mirage::CMaterialData>>;

struct ModelData
{
    static inline FreeListAllocator s_idAllocator;

    static void createBottomLevelAccelStructs(TerrainModelDataEx& terrainModelDataEx);

    static void processSingleElementEffect(InstanceInfoEx& instanceInfoEx,
        Hedgehog::Mirage::CSingleElementEffect* singleElementEffect);

    static void createBottomLevelAccelStructs(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, 
        const MaterialMap& materialMap);

    static void renderSky(Hedgehog::Mirage::CModelData& modelData);

    static void init();
};