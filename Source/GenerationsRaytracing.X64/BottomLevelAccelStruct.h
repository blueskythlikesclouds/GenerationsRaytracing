#pragma once

#include "SubAllocator.h"

struct BottomLevelAccelStruct
{
    SubAllocation allocation;
    uint32_t geometryId = 0;
    uint32_t geometryCount = 0;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc{};
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs{};
    uint32_t scratchBufferSize = 0;
    uint32_t updateScratchBufferSize = 0;
    D3D12_RAYTRACING_INSTANCE_FLAGS instanceFlags{};
    bool asyncBuild = false;
};
