#pragma once

#include "FreeListAllocator.h"
#include "VertexBuffer.h"

class InstanceInfoEx : public Hedgehog::Mirage::CInstanceInfo
{
public:
    uint32_t m_instanceId;
    uint32_t m_bottomLevelAccelStructId;
    ComPtr<VertexBuffer> m_poseVertexBuffer;
    uint32_t m_headNodeIndex;
    bool m_handledEyeMaterials;
    XXH32_hash_t m_modelHash;
    uint32_t m_visibilityBits;
};

struct InstanceData
{
    static inline FreeListAllocator s_idAllocator;

    static void createPendingInstances();
    static void init();
};