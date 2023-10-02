#include "RootSignature.h"

void RootSignature::create(ID3D12Device* device, 
    const D3D12_ROOT_PARAMETER* rootParameters,
    uint32_t rootParamCount,
    D3D12_ROOT_SIGNATURE_FLAGS flags,
    ComPtr<ID3D12RootSignature>& rootSignature)
{
    assert(rootSignature == nullptr);

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = rootParamCount;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = flags;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        blob.GetAddressOf(),
        errorBlob.GetAddressOf());

#ifdef _DEBUG
    if (errorBlob != nullptr)
        OutputDebugStringA(static_cast<LPCSTR>(errorBlob->GetBufferPointer()));
#endif

    assert(SUCCEEDED(hr) && blob != nullptr);

    hr = device->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(rootSignature.GetAddressOf()));

    assert(SUCCEEDED(hr) && rootSignature != nullptr);
}
