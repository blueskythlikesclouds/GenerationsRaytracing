#pragma once

#include "Device.h"

class RaytracingDevice final : public Device
{
protected:
    ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
    ComPtr<ID3D12StateObject> m_stateObject;
    std::vector<uint8_t> m_shaderTable;

    std::vector<ComPtr<D3D12MA::Allocation>> m_bottomLevelAccelStructs;
    std::vector<D3D12_RESOURCE_BARRIER> m_bottomLevelAccelStructBarriers;

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<std::pair<uint32_t, uint32_t>> m_deferredInstances;
    std::vector<std::pair<uint32_t, uint32_t>> m_tempDeferredInstances;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    ComPtr<D3D12MA::Allocation> m_renderTargetTexture;
    ComPtr<D3D12MA::Allocation> m_depthStencilTexture;
    uint32_t m_renderTargetTextureId = 0;
    uint32_t m_depthStencilTextureId = 0;

    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipelineState;

    D3D12_GPU_VIRTUAL_ADDRESS createTopLevelAccelStruct();
    void createRenderTargetAndDepthStencil();
    void copyRenderTargetAndDepthStencil();

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();

    bool processRaytracingMessage() override;

public:
    RaytracingDevice();
};
