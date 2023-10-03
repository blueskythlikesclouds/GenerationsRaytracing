#pragma once

#include "BottomLevelAccelStruct.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "Material.h"

class RaytracingDevice final : public Device
{
protected:
    ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
    ComPtr<ID3D12StateObject> m_stateObject;
    std::vector<uint8_t> m_shaderTable;

    std::vector<BottomLevelAccelStruct> m_bottomLevelAccelStructs;
    std::vector<D3D12_RESOURCE_BARRIER> m_bottomLevelAccelStructBarriers;

    std::vector<GeometryDesc> m_geometryDescs;
    std::multimap<uint32_t, uint32_t> m_freeCounts;
    std::map<uint32_t, std::multimap<uint32_t, uint32_t>::iterator> m_freeIndices;

    std::vector<Material> m_materials;
    std::vector<std::pair<uint32_t, uintptr_t>> m_delayedTextures;
    std::vector<std::pair<uint32_t, uintptr_t>> m_tempDelayedTextures;

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<std::pair<uint32_t, uint32_t>> m_delayedInstances;
    std::vector<std::pair<uint32_t, uint32_t>> m_tempDelayedInstances;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    ComPtr<D3D12MA::Allocation> m_renderTargetTexture;
    ComPtr<D3D12MA::Allocation> m_depthStencilTexture;
    uint32_t m_uavId = 0;

    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipelineState;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);
    D3D12_GPU_VIRTUAL_ADDRESS createTopLevelAccelStruct();
    void createRenderTargetAndDepthStencil();
    void copyRenderTargetAndDepthStencil();

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();
    void procMsgCreateMaterial();

    bool processRaytracingMessage() override;

public:
    RaytracingDevice();
};
