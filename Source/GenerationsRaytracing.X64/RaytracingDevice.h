#pragma once

#include "BottomLevelAccelStruct.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "InstanceDesc.h"
#include "Material.h"
#include "Upscaler.h"

struct alignas(0x10) GlobalsRT
{
    float prevProj[4][4];
    float prevView[4][4];
    float environmentColor[3];
    uint32_t blueNoiseTextureId;
    float pixelJitterX;
    float pixelJitterY;
    uint32_t blueNoiseOffsetX;
    uint32_t blueNoiseOffsetY;
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
    uint32_t m_currentFrame = 0;

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
    std::vector<InstanceDesc> m_instanceDescs2;
    std::vector<std::pair<uint32_t, uint32_t>> m_delayedInstances;
    std::vector<std::pair<uint32_t, uint32_t>> m_tempDelayedInstances;
    ComPtr<D3D12MA::Allocation> m_topLevelAccelStruct;
    std::vector<std::pair<uint32_t, uint32_t>> m_instanceGeometries;

    // Upscaler
    std::unique_ptr<Upscaler> m_upscaler;
    QualityMode m_qualityMode = QualityMode::Balanced;

    // Textures
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    ComPtr<D3D12MA::Allocation> m_colorTexture;
    ComPtr<D3D12MA::Allocation> m_depthTexture;
    ComPtr<D3D12MA::Allocation> m_motionVectorsTexture;
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

    ComPtr<D3D12MA::Allocation> m_outputTexture;
    uint32_t m_srvId = 0;

    // Resolve
    ComPtr<ID3D12PipelineState> m_resolvePipeline;

    // Copy
    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipeline;

    // Compute Pose
    ComPtr<ID3D12RootSignature> m_poseRootSignature;
    ComPtr<ID3D12PipelineState> m_posePipeline;
    std::vector<uint32_t> m_pendingPoses;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    uint32_t buildAccelStruct(ComPtr<D3D12MA::Allocation>& allocation, 
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate);

    void buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct);

    void handlePendingBottomLevelAccelStructBuilds();

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT();
    void createTopLevelAccelStruct();

    void createRaytracingTextures();
    void resolveAndDispatchUpscaler();
    void copyToRenderTargetAndDepthStencil();

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
