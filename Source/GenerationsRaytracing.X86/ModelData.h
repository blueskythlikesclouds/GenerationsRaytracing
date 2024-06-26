#pragma once

#include "FreeListAllocator.h"
#include "InstanceType.h"

class InstanceInfoEx;

class TerrainModelDataEx : public Hedgehog::Mirage::CTerrainModelData
{
public:
    uint32_t m_bottomLevelAccelStructIds[_countof(s_instanceTypes)];
};

class ModelDataEx : public Hedgehog::Mirage::CModelData
{
public:
    uint32_t m_bottomLevelAccelStructIds[_countof(s_instanceTypes)];
    uint32_t m_visibilityFlags;
    XXH32_hash_t m_modelHash;
    uint32_t m_hashFrame;
    bool m_enableSkinning;
    boost::shared_ptr<CModelData> m_noAoModel;
    bool m_checkForEdgeEmission;
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