#pragma once

#include "InstanceDesc.h"
#include "SubAllocator.h"

struct TopLevelAccelStruct
{
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;
    std::vector<InstanceDesc> alsoInstanceDescs;
    SubAllocation allocation;
};