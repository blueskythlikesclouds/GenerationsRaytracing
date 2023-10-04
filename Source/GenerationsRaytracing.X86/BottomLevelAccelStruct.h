#pragma once

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
    static uint32_t create(ModelDataEx& modelDataEx, Hedgehog::Mirage::CPose* pose);

    static void init();
};