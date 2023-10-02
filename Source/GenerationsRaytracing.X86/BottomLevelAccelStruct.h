#pragma once

class TerrainModelDataEx : public Hedgehog::Mirage::CTerrainModelData
{
public:
    uint32_t m_bottomLevelAccelStructId;
};

struct BottomLevelAccelStruct
{
    static void init();
};