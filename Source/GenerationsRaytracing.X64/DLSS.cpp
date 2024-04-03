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

DLSS::DLSS(const NGX& ngx) : m_ngx(ngx)
{
}

DLSS::~DLSS()
{
    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));
}

void DLSS::init(const InitArgs& args)
{
    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));

    NVSDK_NGX_DLSS_Create_Params params{};
    params.Feature.InPerfQualityValue = getPerfQualityValue(args.qualityMode);
    
    uint32_t _;
    
    THROW_IF_FAILED(NGX_DLSS_GET_OPTIMAL_SETTINGS(
        m_ngx.getParameters(),
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

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSS_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        1,
        1,
        &m_feature,
        m_ngx.getParameters(),
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
        m_ngx.getParameters(),
        &params));
}

UpscalerType DLSS::getType()
{
    return UpscalerType::DLSS;
}

bool DLSS::valid(const NGX& ngx)
{
    int32_t needsUpdatedDriver;
    NVSDK_NGX_Result result = ngx.getParameters()->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);

    if (NVSDK_NGX_SUCCEED(result) && needsUpdatedDriver != 0)
        return false;

    int32_t available;
    result = ngx.getParameters()->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &available);

    if (NVSDK_NGX_FAILED(result) || available == 0)
        return false;

    return true;
}
