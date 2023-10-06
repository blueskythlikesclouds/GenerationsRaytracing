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
    static void init();
};