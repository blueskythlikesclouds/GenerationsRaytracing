#pragma once

struct RootSignature
{
    static void create(ID3D12Device* device, 
        const D3D12_ROOT_PARAMETER1* rootParameters,
        uint32_t rootParamCount,
        D3D12_STATIC_SAMPLER_DESC* staticSamplers,
        uint32_t staticSamplerCount,
        D3D12_ROOT_SIGNATURE_FLAGS flags,
        ComPtr<ID3D12RootSignature>& rootSignature);
};