#pragma once
#include "FreeListAllocator.h"

class MaterialDataEx : public Hedgehog::Mirage::CMaterialData
{
public:
    uint32_t m_materialId;
    boost::shared_ptr<float[]> m_originalHighLightParamValue;
    boost::shared_ptr<float[]> m_raytracingHighLightParamValue;
    XXH32_hash_t m_materialHash;
    uint32_t m_hashFrame;
};

struct MaterialData
{
    static inline FreeListAllocator s_idAllocator;

    static bool create(Hedgehog::Mirage::CMaterialData& materialData, bool checkForHash);
    static void createPendingMaterials();

    static void init();
    static void postInit();
};