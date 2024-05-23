#pragma once

struct FeatureCaps
{
    static bool ensureMinimumCapability(ID3D12Device* device, bool enableRaytracing, bool& gpuUploadHeapSupported);
};