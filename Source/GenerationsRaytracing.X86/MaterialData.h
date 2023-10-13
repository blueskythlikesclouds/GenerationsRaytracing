#pragma once
#include "FreeListAllocator.h"

class MaterialDataEx : public Hedgehog::Mirage::CMaterialData
{
public:
    uint32_t m_materialId = 0;
};

struct MaterialData
{
    static inline FreeListAllocator s_idAllocator;

    static bool create(Hedgehog::Mirage::CMaterialData& materialData);
    static void createPendingMaterials();

    static void init();
};