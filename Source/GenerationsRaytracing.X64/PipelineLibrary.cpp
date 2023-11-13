#include "PipelineLibrary.h"

void PipelineLibrary::init(ID3D12Device1* device)
{
    FILE* file = fopen("pipeline_library.bin", "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        m_dataSize = static_cast<size_t>(ftell(file));
        fseek(file, 0, SEEK_SET);

        m_data = std::make_unique<uint8_t[]>(m_dataSize);
        fread(m_data.get(), 1, m_dataSize, file);

        fclose(file);

        const HRESULT hr = device->CreatePipelineLibrary(m_data.get(), m_dataSize, 
            IID_PPV_ARGS(m_pipelineLibrary.GetAddressOf()));

        if (FAILED(hr))
        {
            m_data = nullptr;
            m_dataSize = 0;
        }
    }

    m_device = device;

    if (!m_pipelineLibrary)
    {
        const HRESULT hr = device->CreatePipelineLibrary(nullptr, 0,
            IID_PPV_ARGS(m_pipelineLibrary.GetAddressOf()));

        assert(SUCCEEDED(hr) && m_pipelineLibrary != nullptr);
    }
}

void PipelineLibrary::createGraphicsPipelineState(
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, 
    const IID& iid,
    void** pipelineState)
{
    XXH3_state_t state;
    XXH3_64bits_reset(&state);

    XXH3_64bits_update(&state, desc->VS.pShaderBytecode, desc->VS.BytecodeLength);
    XXH3_64bits_update(&state, desc->PS.pShaderBytecode, desc->PS.BytecodeLength);

    XXH3_64bits_update(&state, &desc->BlendState, sizeof(D3D12_BLEND_DESC));
    XXH3_64bits_update(&state, &desc->RasterizerState, 
        offsetof(D3D12_GRAPHICS_PIPELINE_STATE_DESC, InputLayout) - offsetof(D3D12_GRAPHICS_PIPELINE_STATE_DESC, RasterizerState));

    for (size_t i = 0; i < desc->InputLayout.NumElements; i++)
    {
        auto& inputElement = desc->InputLayout.pInputElementDescs[i];

        XXH3_64bits_update(&state, inputElement.SemanticName, strlen(inputElement.SemanticName));
        XXH3_64bits_update(&state, &inputElement.SemanticIndex,
            sizeof(D3D12_INPUT_ELEMENT_DESC) - offsetof(D3D12_INPUT_ELEMENT_DESC, SemanticIndex));
    }

    XXH3_64bits_update(&state, &desc->IBStripCutValue,
        offsetof(D3D12_GRAPHICS_PIPELINE_STATE_DESC, SampleDesc) - offsetof(D3D12_GRAPHICS_PIPELINE_STATE_DESC, IBStripCutValue));

    wchar_t name[32];
    swprintf_s(name, L"%llX", XXH3_64bits_digest(&state));

    HRESULT hr = m_pipelineLibrary->LoadGraphicsPipeline(name, desc, iid, pipelineState);
    if (FAILED(hr))
    {
        hr = m_device->CreateGraphicsPipelineState(desc, iid, pipelineState);
        assert(SUCCEEDED(hr) && *pipelineState != nullptr);

        hr = m_pipelineLibrary->StorePipeline(name, static_cast<ID3D12PipelineState*>(*pipelineState));
        assert(SUCCEEDED(hr));

        m_dirty = true;
    }
}

void PipelineLibrary::createComputePipelineState(
    const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc, 
    const IID& iid,
    void** pipelineState)
{
    wchar_t name[32];
    swprintf_s(name, L"%llX", XXH3_64bits(desc->CS.pShaderBytecode, desc->CS.BytecodeLength));

    HRESULT hr = m_pipelineLibrary->LoadComputePipeline(name, desc, iid, pipelineState);
    if (FAILED(hr))
    {
        hr = m_device->CreateComputePipelineState(desc, iid, pipelineState);
        assert(SUCCEEDED(hr) && *pipelineState != nullptr);

        hr = m_pipelineLibrary->StorePipeline(name, static_cast<ID3D12PipelineState*>(*pipelineState));
        assert(SUCCEEDED(hr));

        m_dirty = true;
    }
}

void PipelineLibrary::save()
{
    if (m_dirty)
    {
        FILE* file = fopen("pipeline_library.bin", "wb");
        if (file)
        {
            const size_t serializedSize = m_pipelineLibrary->GetSerializedSize();
            const auto serialized = std::make_unique<uint8_t[]>(serializedSize);

            const HRESULT hr = m_pipelineLibrary->Serialize(serialized.get(), serializedSize);
            assert(SUCCEEDED(hr));

            fwrite(serialized.get(), 1, serializedSize, file);
            fclose(file);

            m_dirty = false;
        }
    }
}
