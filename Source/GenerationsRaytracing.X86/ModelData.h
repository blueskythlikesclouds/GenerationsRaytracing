#pragma once

#include "FreeListAllocator.h"

class InstanceInfoEx;

class TerrainModelDataEx : public Hedgehog::Mirage::CTerrainModelData
{
public:
    uint32_t m_bottomLevelAccelStructId;
};

class ModelDataEx : public Hedgehog::Mirage::CModelData
{
public:
    uint32_t m_bottomLevelAccelStructId;
    XXH32_hash_t m_modelHash;
    uint32_t m_visibilityBits;
    uint32_t m_hashFrame;
    boost::shared_ptr<Hedgehog::Mirage::CModelData> m_noAoModel;
};

using MaterialMap = hh::map<Hedgehog::Mirage::CMaterialData*, boost::shared_ptr<Hedgehog::Mirage::CMaterialData>>;

struct ModelData
{
    static inline FreeListAllocator s_idAllocator;

    static void createBottomLevelAccelStruct(TerrainModelDataEx& terrainModelDataEx);

    static void processEyeMaterials(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx,
        MaterialMap& materialMap);

    static void processSingleElementEffect(MaterialMap& materialMap,
        Hedgehog::Mirage::CSingleElementEffect* singleElementEffect);

    static void createBottomLevelAccelStruct(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, 
        const MaterialMap& materialMap);

    static void renderSky(Hedgehog::Mirage::CModelData& modelData);

    static void init();
};