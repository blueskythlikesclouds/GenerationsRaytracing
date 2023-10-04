#pragma once

#include "BottomLevelAccelStruct.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "Material.h"

struct alignas(0x10) GlobalsRT
{
    float environmentColor[3];
    uint32_t currentFrame = 0;
    float pixelJitterX;
    float pixelJitterY;
    uint32_t blueNoiseTextureId;
};

class RaytracingDevice final : public Device
{
protected:
    // Global
    ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
    ComPtr<ID3D12StateObject> m_stateObject;
    std::vector<uint8_t> m_shaderTable;
    GlobalsRT m_globalsRT;

    // Accel Struct
    ComPtr<D3D12MA::Allocation> m_scratchBuffers[NUM_FRAMES];
    uint32_t m_scratchBufferOffset = 0;

    // Bottom Level Accel Struct
    std::vector<BottomLevelAccelStruct> m_bottomLevelAccelStructs;
    std::vector<GeometryDesc> m_geometryDescs;
    std::multimap<uint32_t, uint32_t> m_freeCounts;
    std::map<uint32_t, std::multimap<uint32_t, uint32_t>::iterator> m_freeIndices;

    // Material
    std::vector<Material> m_materials;
    std::vector<std::pair<uint32_t, uintptr_t>> m_delayedTextures;
    std::vector<std::pair<uint32_t, uintptr_t>> m_tempDelayedTextures;

    // Top Level Accel Struct
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<std::pair<uint32_t, uint32_t>> m_delayedInstances;
    std::vector<std::pair<uint32_t, uint32_t>> m_tempDelayedInstances;
    ComPtr<D3D12MA::Allocation> m_topLevelAccelStruct;

    // Textures
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    ComPtr<D3D12MA::Allocation> m_renderTargetTexture;
    ComPtr<D3D12MA::Allocation> m_depthStencilTexture;
    uint32_t m_uavId = 0;

    // Resolve
    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipelineState;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    void buildRaytracingAccelerationStructure(ComPtr<D3D12MA::Allocation>& allocation, 
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc);

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT();
    void createTopLevelAccelStruct();
    void createRenderTargetAndDepthStencil();
    void copyRenderTargetAndDepthStencil();

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();
    void procMsgCreateMaterial();

    bool processRaytracingMessage() override;
    void releaseRaytracingResources() override;

public:
    RaytracingDevice();
};
