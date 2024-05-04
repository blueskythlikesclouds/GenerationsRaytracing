#pragma once
#include "FreeListAllocator.h"

class MaterialDataEx : public Hedgehog::Mirage::CMaterialData
{
public:
    uint32_t m_materialId;
    XXH32_hash_t m_materialHash;
    uint32_t m_hashFrame;
    boost::shared_ptr<CMaterialData> m_fhlMaterial;
};

struct MaterialData
{
    static inline FreeListAllocator s_idAllocator;

    static bool create(Hedgehog::Mirage::CMaterialData& materialData, bool checkForHash);
    static void createPendingMaterials();

    static void init();
    static void postInit();
};