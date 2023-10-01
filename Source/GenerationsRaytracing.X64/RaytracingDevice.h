#pragma once

#include "Device.h"

class RaytracingDevice final : public Device
{
protected:
    std::vector<ComPtr<D3D12MA::Allocation>> m_bottomLevelAccelStructs;

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();

    bool processRaytracingMessage() override;

public:

};
