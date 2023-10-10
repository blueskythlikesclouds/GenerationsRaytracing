#pragma once

class MaterialDataEx : public Hedgehog::Mirage::CMaterialData
{
public:
    uint32_t m_materialId = 0;
};

struct MaterialData
{
    static bool create(Hedgehog::Mirage::CMaterialData& materialData);
    static void handlePendingMaterials();
    static uint32_t allocate();
    static void init();
};