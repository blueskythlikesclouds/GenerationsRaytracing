#pragma once

#include "VertexBuffer.h"

class InstanceInfoEx : public Hedgehog::Mirage::CInstanceInfo
{
public:
    uint32_t m_instanceId;
    uint32_t m_bottomLevelAccelStructId;
    ComPtr<VertexBuffer> m_poseVertexBuffer;
};

struct TopLevelAccelStruct
{
    static uint32_t allocate();
    static void free(uint32_t id);
    static void init();
};