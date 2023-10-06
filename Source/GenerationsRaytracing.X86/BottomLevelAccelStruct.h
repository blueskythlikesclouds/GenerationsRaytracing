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

struct BottomLevelAccelStruct
{
    static void create(ModelDataEx& modelDataEx, InstanceInfoEx& instanceInfoEx);
    static void release(uint32_t bottomLevelAccelStructId);

    static void init();
};