#pragma once

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

struct BottomLevelAccelStruct
{
    static uint32_t allocate();
    static void create(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx, const MaterialMap& materialMap, bool isEnabled);
    static void release(uint32_t bottomLevelAccelStructId);

    static void init();
};