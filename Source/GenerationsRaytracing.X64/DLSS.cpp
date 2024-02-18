#include "DLSS.h"
#include "Device.h"

NVSDK_NGX_PerfQuality_Value DLSS::getPerfQualityValue(QualityMode qualityMode)
{
    switch (qualityMode)
    {
    case QualityMode::Quality: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
    case QualityMode::Balanced: return NVSDK_NGX_PerfQuality_Value_Balanced;
    case QualityMode::Performance: return NVSDK_NGX_PerfQuality_Value_MaxPerf;
    case QualityMode::UltraPerformance: return NVSDK_NGX_PerfQuality_Value_UltraPerformance;
    }

    return NVSDK_NGX_PerfQuality_Value_DLAA;
}

DLSS::DLSS(const Device& device)
{
    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_Init(0x205452736E654720, L".", device.getUnderlyingDevice())))
        return;

    m_device = device.getUnderlyingDevice();

    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&m_parameters)))
        return;

    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
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
    NVSDK_NGX_DLSS_Create_Params params{};
    params.Feature.InPerfQualityValue = getPerfQualityValue(args.qualityMode);
    
    uint32_t _;
    
    THROW_IF_FAILED(NGX_DLSS_GET_OPTIMAL_SETTINGS(
        m_parameters,
        args.width,
        args.height,
        params.Feature.InPerfQualityValue,
        &m_width,
        &m_height,
        &_,
        &_,
        &_,
        &_,
        reinterpret_cast<float*>(&_)));
    
    assert(m_width > 0 && m_height > 0);

    params.Feature.InWidth = m_width;
    params.Feature.InHeight = m_height;
    params.Feature.InTargetWidth = args.width;
    params.Feature.InTargetHeight = args.height;
    params.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSS_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        1,
        1,
        &m_feature,
        m_parameters,
        &params));
}

void DLSS::dispatch(const DispatchArgs& args)
{
    NVSDK_NGX_D3D12_DLSS_Eval_Params params{};

    params.Feature.pInColor = args.color;
    params.Feature.pInOutput = args.output;
    params.pInDepth = args.depth;
    params.pInMotionVectors = args.motionVectors;
    params.InJitterOffsetX = args.jitterX;
    params.InJitterOffsetY = args.jitterY;
    params.InRenderSubrectDimensions.Width = m_width;
    params.InRenderSubrectDimensions.Height = m_height;
    params.InReset = args.resetAccumulation;
    params.pInExposureTexture = args.exposure;
    params.InFrameTimeDeltaInMsec = args.device.getGlobalsPS().floatConstants[68][0] * 1000.0f;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSS_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        m_feature, 
        m_parameters, 
        &params));
}

UpscalerType DLSS::getType()
{
    return UpscalerType::DLSS;
}

bool DLSS::valid() const
{
    if (m_parameters == nullptr)
        return false;

    int32_t needsUpdatedDriver;
    NVSDK_NGX_Result result = m_parameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);

    if (NVSDK_NGX_SUCCEED(result) && needsUpdatedDriver != 0)
        return false;

    int32_t available;
    result = m_parameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &available);

    if (NVSDK_NGX_FAILED(result) || available == 0)
        return false;

    return true;
}
