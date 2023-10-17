#pragma once

#include "BottomLevelAccelStruct.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "InstanceDesc.h"
#include "LocalLight.h"
#include "Material.h"
#include "Upscaler.h"

struct alignas(0x10) GlobalsRT
{
    float prevProj[4][4];
    float prevView[4][4];
    float skyColor[3];
    uint32_t skyTextureId;
    float groundColor[3];
    uint32_t useEnvironmentColor;
    float pixelJitterX;
    float pixelJitterY;
    uint32_t blueNoiseOffsetX;
    uint32_t blueNoiseOffsetY;
    uint32_t blueNoiseTextureId;
    uint32_t localLightCount;
    uint32_t currentFrame = 0;
};

struct DelayedTexture
{
    uint32_t materialId;
    uint32_t materialVersion;
    uint32_t textureId;
    uint32_t textureIdOffset;
};

struct GlobalsSB
{
    float proj[4][4];
    float view[4][4];
    float backgroundScale;
    uint32_t diffuseTextureId;
    uint32_t alphaTextureId;
    uint32_t emissionTextureId;
    float ambient[3];
    uint32_t enableAlphaTest;
    float texCoordOffsets[8];
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
    std::vector<DelayedTexture> m_delayedTextures[NUM_FRAMES];

    // Top Level Accel Struct
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<std::pair<uint32_t, uint32_t>> m_delayedInstances[NUM_FRAMES];
    std::vector<InstanceDesc> m_instanceDescsEx;
    std::vector<std::pair<uint32_t, uint32_t>> m_geometryRanges;
    ComPtr<D3D12MA::Allocation> m_topLevelAccelStruct;

    // Upscaler
    std::unique_ptr<Upscaler> m_upscaler;
    QualityMode m_qualityMode = QualityMode::Balanced;

    // Textures
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_uavIds[NUM_FRAMES]{};

    ComPtr<D3D12MA::Allocation> m_colorTexture;
    ComPtr<D3D12MA::Allocation> m_depthTexture;
    ComPtr<D3D12MA::Allocation> m_prevDepthTexture;
    ComPtr<D3D12MA::Allocation> m_motionVectorsTexture;

    ComPtr<D3D12MA::Allocation> m_positionFlagsTexture;
    ComPtr<D3D12MA::Allocation> m_prevPositionFlagsTexture;
    ComPtr<D3D12MA::Allocation> m_diffuseTexture;
    ComPtr<D3D12MA::Allocation> m_specularTexture;
    ComPtr<D3D12MA::Allocation> m_specularPowerLevelFresnelTexture;
    ComPtr<D3D12MA::Allocation> m_normalTexture;
    ComPtr<D3D12MA::Allocation> m_prevNormalTexture;
    ComPtr<D3D12MA::Allocation> m_falloffTexture;
    ComPtr<D3D12MA::Allocation> m_emissionTexture;

    ComPtr<D3D12MA::Allocation> m_shadowTexture;
    ComPtr<D3D12MA::Allocation> m_diReservoirTexture;
    ComPtr<D3D12MA::Allocation> m_prevDIReservoirTexture;

    ComPtr<D3D12MA::Allocation> m_giTexture;
    ComPtr<D3D12MA::Allocation> m_giPositionTexture;
    ComPtr<D3D12MA::Allocation> m_giNormalTexture;

    ComPtr<D3D12MA::Allocation> m_prevGITexture;
    ComPtr<D3D12MA::Allocation> m_prevGIPositionTexture;
    ComPtr<D3D12MA::Allocation> m_prevGINormalTexture;

    ComPtr<D3D12MA::Allocation> m_giAccumulationTexture;
    ComPtr<D3D12MA::Allocation> m_prevGIAccumulationTexture;

    ComPtr<D3D12MA::Allocation> m_reflectionTexture;
    ComPtr<D3D12MA::Allocation> m_refractionTexture;

    ComPtr<D3D12MA::Allocation> m_outputTexture;
    uint32_t m_srvIds[NUM_FRAMES]{};

    // Resolve
    ComPtr<ID3D12PipelineState> m_resolvePipeline;

    // Copy
    ComPtr<ID3D12RootSignature> m_copyTextureRootSignature;
    ComPtr<ID3D12PipelineState> m_copyTexturePipeline;

    // Compute Pose
    ComPtr<ID3D12RootSignature> m_poseRootSignature;
    ComPtr<ID3D12PipelineState> m_posePipeline;
    std::vector<uint32_t> m_pendingPoses;

    // Sky
    ComPtr<ID3D12RootSignature> m_skyRootSignature;
    ComPtr<ID3D12Resource> m_skyTexture;
    uint32_t m_skyRtvIndices[6]{};

    // Local Light
    std::vector<LocalLight> m_localLights;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    uint32_t buildAccelStruct(ComPtr<D3D12MA::Allocation>& allocation, 
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate);

    void buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct);

    void handlePendingBottomLevelAccelStructBuilds();

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT(uint32_t localLightCount);
    bool createTopLevelAccelStruct();

    void createRaytracingTextures();
    void resolveAndDispatchUpscaler(bool resetAccumulation);
    void copyToRenderTargetAndDepthStencil();

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();
    void procMsgCreateMaterial();
    void procMsgBuildBottomLevelAccelStruct();
    void procMsgComputePose();
    void procMsgRenderSky();
    void procMsgCreateLocalLight();

    bool processRaytracingMessage() override;
    void releaseRaytracingResources() override;

public:
    RaytracingDevice();
};
