#include "DLSS.h"
#include "Device.h"

#ifdef _DEBUG

#define THROW_IF_FAILED(x) \
    do \
    { \
        const NVSDK_NGX_Result result = x; \
        if (NVSDK_NGX_FAILED(result)) \
        { \
            OutputDebugStringW(GetNGXResultAsString(result)); \
            assert(!#x); \
        } \
    } while (0)

#else
#define THROW_IF_FAILED(x) x
#endif

DLSS::DLSS(const Device& device)
{
    if (NVSDK_NGX_SUCCEED(NVSDK_NGX_D3D12_Init(0x205452736E654720, L".", device.getUnderlyingDevice())) &&
        NVSDK_NGX_SUCCEED(NVSDK_NGX_D3D12_GetCapabilityParameters(&m_parameters)))
    {
        m_device = device.getUnderlyingDevice();
    }
}

DLSS::~DLSS()
{
    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));

    if (m_parameters != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(m_parameters));

    if (m_device != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown1(m_device.Get()));
}

void DLSS::init(const InitArgs& args)
{
    NVSDK_NGX_DLSSD_Create_Params params{};

    params.InPerfQualityValue =
        args.qualityMode == QualityMode::Quality ? NVSDK_NGX_PerfQuality_Value_MaxQuality :
        args.qualityMode == QualityMode::Balanced ? NVSDK_NGX_PerfQuality_Value_Balanced :
        args.qualityMode == QualityMode::Performance ? NVSDK_NGX_PerfQuality_Value_MaxPerf :
        args.qualityMode == QualityMode::UltraPerformance ? NVSDK_NGX_PerfQuality_Value_UltraPerformance : NVSDK_NGX_PerfQuality_Value_DLAA;
    
    uint32_t _;
    
    THROW_IF_FAILED(NGX_DLSSD_GET_OPTIMAL_SETTINGS(
        m_parameters,
        args.width,
        args.height,
        params.InPerfQualityValue,
        &m_width,
        &m_height,
        &_,
        &_,
        &_,
        &_,
        &m_sharpness));
    
    assert(m_width > 0 && m_height > 0);

    params.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
    params.InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Packed;
    params.InUseHWDepth = NVSDK_NGX_DLSS_Depth_Type_HW;
    params.InWidth = m_width;
    params.InHeight = m_height;
    params.InTargetWidth = args.width;
    params.InTargetHeight = args.height;
    params.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSSD_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        1,
        1,
        &m_feature,
        m_parameters,
        &params));
}

void DLSS::dispatch(const DispatchArgs& args)
{
    NVSDK_NGX_D3D12_DLSSD_Eval_Params params{};

    params.pInDiffuseAlbedo = args.diffuseAlbedo;
    params.pInSpecularAlbedo = args.specularAlbedo;
    params.pInNormals = args.normalsRoughness;
    params.pInColor = args.color;
    params.pInOutput = args.output;
    params.pInDepth = args.depth;
    params.pInMotionVectors = args.motionVectors;
    params.InJitterOffsetX = args.jitterX;
    params.InJitterOffsetY = args.jitterY;
    params.InRenderSubrectDimensions.Width = m_width;
    params.InRenderSubrectDimensions.Height = m_height;
    params.InReset = args.resetAccumulation;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSSD_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        m_feature, 
        m_parameters, 
        &params));
}

bool DLSS::valid() const
{
    return m_device != nullptr;
}
