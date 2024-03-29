#pragma once

#include "BottomLevelAccelStruct.h"
#include "DebugView.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "InstanceDesc.h"
#include "LocalLight.h"
#include "Material.h"
#include "ShaderType.h"
#include "Upscaler.h"

struct MsgTraceRays;

struct alignas(0x10) GlobalsRT
{
    float prevProj[4][4];
    float prevView[4][4];
    float skyColor[3];
    uint32_t skyTextureId;
    float backgroundColor[3];
    uint32_t useSkyTexture;
    float groundColor[3];
    uint32_t useEnvironmentColor;
    float pixelJitterX;
    float pixelJitterY;
    uint32_t internalResolutionWidth;
    uint32_t internalResolutionHeight;
    uint32_t blueNoiseOffsetX;
    uint32_t blueNoiseOffsetY;
    uint32_t blueNoiseOffsetZ;
    uint32_t blueNoiseTextureId;
    uint32_t localLightCount;
    uint32_t currentFrame = 0;
    float diffusePower;
    float lightPower;
    float emissivePower;
    float skyPower;
    uint32_t adaptionLuminanceTextureId;
    float middleGray;
};

struct GlobalsSB
{
    float backgroundScale;
    uint32_t diffuseTextureId;
    uint32_t alphaTextureId;
    uint32_t emissionTextureId;
    float diffuse[3];
    uint32_t enableVertexColor;
    float ambient[3];
    uint32_t enableAlphaTest;
};

struct SmoothNormalCmd
{
    uint32_t indexBufferId;
    uint32_t vertexStride;
    uint32_t vertexCount;
    uint32_t vertexOffset;
    uint32_t normalOffset;
    uint32_t vertexBufferId;
    uint32_t adjacencyBufferId;
};

inline constexpr size_t HIT_GROUP_NUM = 2;

class RaytracingDevice final : public Device
{
protected:
    // Global
    ID3D12RootSignature* m_curRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
    ComPtr<ID3D12StateObject> m_stateObject;
    ComPtr<ID3D12StateObjectProperties> m_properties;
    size_t m_primaryStackSize = 0;
    size_t m_shadowStackSize = 0;
    size_t m_giStackSize = 0;
    size_t m_reflectionStackSize = 0;
    size_t m_refractionStackSize = 0;
    void* m_hitGroups[_countof(s_shaderHitGroups)]{};
    std::vector<uint8_t> m_rayGenShaderTable;
    std::vector<uint8_t> m_missShaderTable;
    std::vector<uint8_t> m_hitGroupShaderTable;
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

    // Top Level Accel Struct
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<InstanceDesc> m_instanceDescsEx;
    std::vector<std::pair<uint32_t, uint32_t>> m_geometryRanges;
    ComPtr<D3D12MA::Allocation> m_topLevelAccelStruct;

    // Upscaler
    std::unique_ptr<Upscaler> m_upscaler;
    QualityMode m_qualityMode = QualityMode::Balanced;
    UpscalerType m_upscalerOverride[static_cast<size_t>(UpscalerType::Max)];

    // Textures
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_uavId = 0;

    ComPtr<D3D12MA::Allocation> m_colorTexture;
    ComPtr<D3D12MA::Allocation> m_depthTexture;
    ComPtr<D3D12MA::Allocation> m_motionVectorsTexture;
    ComPtr<D3D12MA::Allocation> m_exposureTexture;

    ComPtr<D3D12MA::Allocation> m_gBufferTexture0;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture1;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture2;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture3;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture4;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture5;
    ComPtr<D3D12MA::Allocation> m_gBufferTexture6;

    ComPtr<D3D12MA::Allocation> m_shadowTexture;
    ComPtr<D3D12MA::Allocation> m_reservoirTexture;
    ComPtr<D3D12MA::Allocation> m_prevReservoirTexture;

    ComPtr<D3D12MA::Allocation> m_giTexture;
    ComPtr<D3D12MA::Allocation> m_reflectionTexture;
    ComPtr<D3D12MA::Allocation> m_refractionTexture;

    ComPtr<D3D12MA::Allocation> m_diffuseAlbedoTexture;
    ComPtr<D3D12MA::Allocation> m_specularAlbedoTexture;
    ComPtr<D3D12MA::Allocation> m_linearDepthTexture;
    ComPtr<D3D12MA::Allocation> m_diffuseHitDistanceTexture;
    ComPtr<D3D12MA::Allocation> m_specularHitDistanceTexture;

    ComPtr<D3D12MA::Allocation> m_outputTexture;
    uint32_t m_srvIds[DEBUG_VIEW_MAX]{};
    ID3D12Resource* m_debugViewTextures[DEBUG_VIEW_MAX]{};

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
    uint32_t m_skyRtvId{};

    // Local Light
    std::vector<LocalLight> m_localLights;

    // Smooth Normal
    ComPtr<ID3D12RootSignature> m_smoothNormalRootSignature;
    ComPtr<ID3D12PipelineState> m_smoothNormalPipeline;
    std::vector<SmoothNormalCmd> m_smoothNormalCommands;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    uint32_t buildAccelStruct(ComPtr<D3D12MA::Allocation>& allocation, 
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate);

    void buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct);

    void handlePendingBottomLevelAccelStructBuilds();

    void handlePendingSmoothNormalCommands();

    void writeHitGroupShaderTable(size_t geometryIndex, size_t shaderType, bool constTexCoord);

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT(const MsgTraceRays& message);
    bool createTopLevelAccelStruct();

    void createRaytracingTextures();
    void resolveAndDispatchUpscaler(const MsgTraceRays& message);
    void copyToRenderTargetAndDepthStencil(const MsgTraceRays& message);

    void procMsgCreateBottomLevelAccelStruct();
    void procMsgReleaseRaytracingResource();
    void procMsgCreateInstance();
    void procMsgTraceRays();
    void procMsgCreateMaterial();
    void procMsgBuildBottomLevelAccelStruct();
    void procMsgComputePose();
    void procMsgRenderSky();
    void procMsgCreateLocalLight();
    void procMsgComputeSmoothNormal();

    bool processRaytracingMessage() override;
    void releaseRaytracingResources() override;

public:
    RaytracingDevice();
    ~RaytracingDevice() override;
};
