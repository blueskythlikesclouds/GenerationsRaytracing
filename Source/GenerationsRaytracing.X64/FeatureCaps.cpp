#include "FeatureCaps.h"

bool FeatureCaps::ensureMinimumCapability(ID3D12Device* device)
{
    CD3DX12FeatureSupport features;
    features.Init(device);

    return features.RaytracingTier() >= D3D12_RAYTRACING_TIER_1_1 &&
        features.HighestShaderModel() >= D3D_SHADER_MODEL_6_6 &&
        features.ResourceBindingTier() >= D3D12_RESOURCE_BINDING_TIER_2 &&
        features.HighestRootSignatureVersion() >= D3D_ROOT_SIGNATURE_VERSION_1_1;
}
