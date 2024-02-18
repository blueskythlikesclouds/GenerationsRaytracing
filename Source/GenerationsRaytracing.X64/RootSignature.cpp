#include "RootSignature.h"

void RootSignature::create(ID3D12Device* device, 
    const D3D12_ROOT_PARAMETER1* rootParameters,
    uint32_t rootParamCount,
    D3D12_STATIC_SAMPLER_DESC* staticSamplers,
    uint32_t staticSamplerCount,
    D3D12_ROOT_SIGNATURE_FLAGS flags,
    ComPtr<ID3D12RootSignature>& rootSignature,
    const char* rootSignatureName)
{
    assert(rootSignature == nullptr);

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSignatureDesc.Desc_1_1.NumParameters = rootParamCount;
    rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = staticSamplerCount;
    rootSignatureDesc.Desc_1_1.pStaticSamplers = staticSamplers;
    rootSignatureDesc.Desc_1_1.Flags = flags |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeVersionedRootSignature(
        &rootSignatureDesc,
        blob.GetAddressOf(),
        errorBlob.GetAddressOf());

#ifdef _DEBUG
    if (errorBlob != nullptr)
        OutputDebugStringA(static_cast<LPCSTR>(errorBlob->GetBufferPointer()));
#else
    if (errorBlob != nullptr)
        MessageBoxA(nullptr, static_cast<LPCSTR>(errorBlob->GetBufferPointer()), "Generations Raytracing", MB_ICONERROR);
#endif

    assert(SUCCEEDED(hr) && blob != nullptr);

#if 0
    char fileName[0x100];
    sprintf(fileName, "root_signature_%s.bin", rootSignatureName);
    FILE* file = fopen(fileName, "wb");
    fwrite(blob->GetBufferPointer(), 1, blob->GetBufferSize(), file);
    fclose(file);
#endif

    hr = device->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature.GetAddressOf()));

    assert(SUCCEEDED(hr) && rootSignature != nullptr);
}
