#pragma once

#include "BottomLevelAccelStruct.h"
#include "DebugView.h"
#include "Device.h"
#include "GeometryDesc.h"
#include "InstanceDesc.h"
#include "LocalLight.h"
#include "Material.h"
#include "NGX.h"
#include "HitGroups.h"
#include "SubAllocator.h"
#include "Upscaler.h"

struct MsgDispatchUpscaler;
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
    uint32_t currentFrame = 0;
    uint32_t randomSeed;
    uint32_t localLightCount;
    float diffusePower;
    float lightPower;
    float emissivePower;
    float skyPower;
    uint32_t adaptionLuminanceTextureId;
    float middleGray;
    uint32_t skyInRoughReflection;
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
    uint32_t indexOffset;
    uint32_t vertexStride;
    uint32_t vertexCount;
    uint32_t vertexOffset;
    uint32_t normalOffset;
    uint32_t vertexBufferId;
    uint32_t adjacencyBufferId;
};

class RaytracingDevice final : public Device
{
protected:
    // Global
    ID3D12RootSignature* m_curRootSignature = nullptr;
    ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
    ComPtr<ID3D12StateObject> m_stateObject;
    ComPtr<ID3D12StateObjectProperties> m_properties;
    size_t m_primaryStackSize = 0;
    size_t m_secondaryStackSize = 0;
    size_t m_shadowStackSize = 0;
    void* m_hitGroups[_countof(s_shaderHitGroups)]{};
    std::vector<uint8_t> m_rayGenShaderTable;
    std::vector<uint8_t> m_missShaderTable;
    std::vector<uint8_t> m_hitGroupShaderTable;
    GlobalsRT m_globalsRT;

    bool m_serSupported = false;
    uint32_t m_serUavId = 0;

    // Accel Struct
    ComPtr<D3D12MA::Allocation> m_scratchBuffers[NUM_FRAMES];
    uint32_t m_scratchBufferOffset = 0;

    // Bottom Level Accel Struct
    SubAllocator m_bottomLevelAccelStructAllocator{
        DEFAULT_BLOCK_SIZE,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE };

    std::vector<BottomLevelAccelStruct> m_bottomLevelAccelStructs;
    std::vector<GeometryDesc> m_geometryDescs;
    std::multimap<uint32_t, uint32_t> m_freeCounts;
    std::map<uint32_t, std::multimap<uint32_t, uint32_t>::iterator> m_freeIndices;
    std::vector<uint32_t> m_pendingBuilds;
    std::vector<SubAllocation> m_tempBottomLevelAccelStructs[NUM_FRAMES];

    // Material
    std::vector<Material> m_materials;

    // Top Level Accel Struct
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> m_instanceDescs;
    std::vector<InstanceDesc> m_instanceDescsEx;
    std::vector<std::pair<uint32_t, uint32_t>> m_geometryRanges;
    SubAllocation m_topLevelAccelStruct;

    // Upscaler
    std::unique_ptr<NGX> m_ngx;
    std::unique_ptr<Upscaler> m_upscaler;
    QualityMode m_qualityMode = QualityMode::Balanced;
    UpscalerType m_upscalerOverride[static_cast<size_t>(UpscalerType::Max)];

    // Textures
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    ComPtr<D3D12MA::Allocation> m_dispatchRaysIndexBuffer;

    uint32_t m_uavId = 0;
    uint32_t m_srvId = 0;
    uint32_t m_exposureTextureId = 0;

    ComPtr<D3D12MA::Allocation> m_colorTexture;
    ComPtr<D3D12MA::Allocation> m_depthTexture;
    ComPtr<D3D12MA::Allocation> m_motionVectorsTexture;
    ComPtr<D3D12MA::Allocation> m_exposureTexture;

    ComPtr<D3D12MA::Allocation> m_layerNumTexture;

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

    ComPtr<D3D12MA::Allocation> m_diffuseAlbedoTexture;
    ComPtr<D3D12MA::Allocation> m_specularAlbedoTexture;
    ComPtr<D3D12MA::Allocation> m_linearDepthTexture;
    ComPtr<D3D12MA::Allocation> m_colorBeforeTransparencyTexture;
    ComPtr<D3D12MA::Allocation> m_specularHitDistanceTexture;

    ComPtr<D3D12MA::Allocation> m_outputTexture;
    uint32_t m_srvIds[DEBUG_VIEW_MAX]{};
    ID3D12Resource* m_debugViewTextures[DEBUG_VIEW_MAX]{};

    // Resolve
    ComPtr<ID3D12PipelineState> m_resolvePipeline;
    ComPtr<ID3D12PipelineState> m_resolveTransparencyPipeline;

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

    // Im3d
    ComPtr<ID3D12RootSignature> m_im3dRootSignature;
    ComPtr<ID3D12PipelineState> m_im3dTrianglesPipeline;
    ComPtr<ID3D12PipelineState> m_im3dLinesPipeline;
    ComPtr<ID3D12PipelineState> m_im3dPointsPipeline;

    uint32_t allocateGeometryDescs(uint32_t count);
    void freeGeometryDescs(uint32_t id, uint32_t count);

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO buildAccelStruct(SubAllocation& allocation,
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& accelStructDesc, bool buildImmediate);

    void buildBottomLevelAccelStruct(BottomLevelAccelStruct& bottomLevelAccelStruct);

    void handlePendingBottomLevelAccelStructBuilds();

    void handlePendingSmoothNormalCommands();

    void writeHitGroupShaderTable(size_t geometryIndex, size_t shaderType, bool constTexCoord);

    D3D12_GPU_VIRTUAL_ADDRESS createGlobalsRT(const MsgTraceRays& message);
    bool createTopLevelAccelStruct();

    void createRaytracingTextures();
    void dispatchResolver(const MsgTraceRays& message);
    void copyToRenderTargetAndDepthStencil(const MsgDispatchUpscaler& message);
    void prepareForDispatchUpscaler(const MsgTraceRays& message);

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
    void procMsgDispatchUpscaler();
    void procMsgDrawIm3d();
    void procMsgUpdatePlayableParam();

    bool processRaytracingMessage() override;
    void releaseRaytracingResources() override;

public:
    RaytracingDevice();
    ~RaytracingDevice() override;
};
