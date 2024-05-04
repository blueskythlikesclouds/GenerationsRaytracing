#pragma once

#include "FreeListAllocator.h"
#include "InstanceMask.h"
#include "VertexBuffer.h"

class TerrainInstanceInfoDataEx : public Hedgehog::Mirage::CTerrainInstanceInfoData
{
public:
    uint32_t m_instanceFrames[_countof(s_instanceMasks)];
    float m_prevTransform[3][4];
    std::unordered_multimap<uint32_t, TerrainInstanceInfoDataEx*>::iterator m_subsetIterator;
    bool m_hasValidIterator;
};

class InstanceInfoEx : public Hedgehog::Mirage::CInstanceInfo
{
public:
    uint32_t m_instanceFrames[_countof(s_instanceMasks)];
    float m_prevTransform[3][4];
    uint32_t m_bottomLevelAccelStructIds[_countof(s_instanceMasks)];
    ComPtr<VertexBuffer> m_poseVertexBuffer;
    uint32_t m_headNodeIndex;
    bool m_handledEyeMaterials;
    XXH32_hash_t m_modelHash;
    uint64_t m_hashFrame;
    float m_chrPlayableMenuParam;
    std::unordered_map<Hedgehog::Mirage::CMaterialData*, boost::shared_ptr<Hedgehog::Mirage::CMaterialData>> m_effectMap;
    XXH32_hash_t m_matrixHash;
    XXH32_hash_t m_prevMatrixHash;
    bool m_enableForceAlphaColor;
    float m_forceAlphaColor;
};

struct InstanceData
{
    static void createInstances(Hedgehog::Mirage::CRenderingDevice* renderingDevice);

    static void init();
};