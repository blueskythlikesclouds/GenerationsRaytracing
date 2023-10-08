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
    uint32_t blueNoiseOffsetX;
    uint32_t blueNoiseOffsetY;
    uint32_t blueNoiseTextureId;
};

class RaytracingDevice final : public Device
{
protected:
    // Global
    ID3D12RootSignature* m_curRootSignature = nullptr;
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
    std::vector<uint32_t> m_pendingBuilds;

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
    ComPtr<D3D12MA::Allocation> m_positionAndFlagsTexture;
    ComPtr<D3D12MA::Allocation> m_normalTexture;
    ComPtr<D3D12MA::Allocation> m_diffuseTexture;
    ComPtr<D3D12MA::Allocation> m_specularTexture;
    ComPtr<D3D12MA::Allocation> m_specularPowerTexture;
    ComPtr<D3D12MA::Allocation> m_specularLevelTexture;
    ComPtr<D3D12MA::Allocation> m_emissionTexture;
    ComPtr<D3D12MA::Allocation> m_falloffTexture;
    ComPtr<D3D12MA::Allocation> m_shadowTexture;
    ComPtr<D3D12MA::Allocation> m_globalIlluminationTexture;
    ComPtr<D3D12MA::Allocation> m_reflectionTexture;
    uint32_t m_uavId = 0;

    // Resolve
    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipeline;

    // Compute Pose
    ComPtr<ID3D12RootSignature> m_poseRootSignature;
    ComPtr<ID3D12PipelineState> m_posePipeline;
    std::vector<uint32_t> m_pendingPoses;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    uint32_t buildRaytracingAccelerationStructure(ComPtr<D3D12MA::Allocation>& allocation, 
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate);

    void handlePendingBottomLevelAccelStructBuilds();

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT();
    void createTopLevelAccelStruct();
    void createRenderTargetAndDepthStencil();
    void resolveDeferredShading(D3D12_GPU_VIRTUAL_ADDRESS globalsVS, D3D12_GPU_VIRTUAL_ADDRESS globalsPS);

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();
    void procMsgCreateMaterial();
    void procMsgBuildBottomLevelAccelStruct();
    void procMsgComputePose();

    bool processRaytracingMessage() override;
    void releaseRaytracingResources() override;

public:
    RaytracingDevice();
};
