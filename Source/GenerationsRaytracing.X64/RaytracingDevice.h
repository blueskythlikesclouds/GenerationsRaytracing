#pragma once

#include "Device.h"

class RaytracingDevice final : public Device
{
protected:
    std::vector<ComPtr<D3D12MA::Allocation>> m_bottomLevelAccelStructs;

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<std::pair<uint32_t, uint32_t>> m_deferredInstances;
    std::vector<std::pair<uint32_t, uint32_t>> m_tempDeferredInstances;

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();

    bool processRaytracingMessage() override;

    void createTopLevelAccelStruct();
    void updateRaytracing() override;

public:

};
