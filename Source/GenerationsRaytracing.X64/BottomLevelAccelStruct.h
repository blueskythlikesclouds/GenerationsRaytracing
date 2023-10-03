#pragma once

struct BottomLevelAccelStruct
{
    ComPtr<D3D12MA::Allocation> allocation;
    uint32_t geometryId = 0;
    uint32_t geometryCount = 0;
};