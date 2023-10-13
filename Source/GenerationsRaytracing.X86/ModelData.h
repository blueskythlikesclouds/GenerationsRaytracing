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
};

using MaterialMap = hh::map<Hedgehog::Mirage::CMaterialData*, boost::shared_ptr<Hedgehog::Mirage::CMaterialData>>;

struct ModelData
{
    static inline FreeListAllocator s_idAllocator;

    static void createBottomLevelAccelStruct(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, 
        const MaterialMap& materialMap, bool isEnabled);

    static void renderSky(Hedgehog::Mirage::CModelData& modelData);

    static void init();
};