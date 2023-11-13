#pragma once

class PipelineLibrary
{
protected:
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12PipelineLibrary> m_pipelineLibrary;
    std::unique_ptr<uint8_t[]> m_data;
    size_t m_dataSize = 0;
    bool m_dirty = false;

public:
    void init(ID3D12Device1* device);

    void createGraphicsPipelineState(
        const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc,
        REFIID iid,
        void** pipelineState);

    void createComputePipelineState(
        const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
        REFIID iid,
        void** pipelineState);

    void save();
};