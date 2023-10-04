#pragma once

class InstanceInfoEx : public Hedgehog::Mirage::CInstanceInfo
{
public:
    uint32_t m_instanceId;
    uint32_t m_bottomLevelAccelStructId;
};

struct TopLevelAccelStruct
{
    static void init();
};