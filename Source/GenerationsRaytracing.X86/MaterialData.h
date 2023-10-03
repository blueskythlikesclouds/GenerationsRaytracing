#pragma once

class MaterialDataEx : public Hedgehog::Mirage::CMaterialData
{
public:
    uint32_t m_materialId = 0;
};

struct MaterialData
{
    static void init();
};