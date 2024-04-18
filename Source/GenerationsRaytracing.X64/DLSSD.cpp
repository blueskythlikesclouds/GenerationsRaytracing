#include "DLSSD.h"
#include "Device.h"

DLSSD::DLSSD(const NGX& ngx) : DLSS(ngx)
{
}

void DLSSD::init(const InitArgs& args)
{
    if (m_feature != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(m_feature));

    NVSDK_NGX_DLSSD_Create_Params params{};
    params.InPerfQualityValue = getPerfQualityValue(args.qualityMode);
    
    uint32_t _;
    
    THROW_IF_FAILED(NGX_DLSSD_GET_OPTIMAL_SETTINGS(
        m_ngx.getParameters(),
        args.width,
        args.height,
        params.InPerfQualityValue,
        &m_width,
        &m_height,
        &_,
        &_,
        &_,
        &_,
        reinterpret_cast<float*>(&_)));
    
    assert(m_width > 0 && m_height > 0);

    const uint32_t widthAlignedUp = (m_width + 31) & ~31;
    const uint32_t heightAlignedUp = (m_height + 31) & ~31;

    const uint32_t widthAlignedDown = m_width & ~31;
    const uint32_t heightAlignedDown = m_height & ~31;

    if (widthAlignedUp > args.width || (m_width - widthAlignedDown) < (widthAlignedUp - m_width))
        m_width = widthAlignedDown;
    else
        m_width = widthAlignedUp;

    if (heightAlignedUp > args.height || (m_height - heightAlignedDown) < (heightAlignedUp - m_height))
        m_height = heightAlignedDown;
    else
        m_height = heightAlignedUp;

    params.InDenoiseMode = NVSDK_NGX_DLSS_Denoise_Mode_DLUnified;
    params.InRoughnessMode = NVSDK_NGX_DLSS_Roughness_Mode_Packed;
    params.InWidth = m_width;
    params.InHeight = m_height;
    params.InTargetWidth = args.width;
    params.InTargetHeight = args.height;
    params.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSSD_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        1,
        1,
        &m_feature,
        m_ngx.getParameters(),
        &params));
}

void DLSSD::dispatch(const DispatchArgs& args)
{
    NVSDK_NGX_D3D12_DLSSD_Eval_Params params{};

    params.pInDiffuseAlbedo = args.diffuseAlbedo;
    params.pInSpecularAlbedo = args.specularAlbedo;
    params.pInNormals = args.normals;
    params.pInColor = args.color;
    params.pInOutput = args.output;
    params.pInDepth = args.linearDepth;
    params.pInMotionVectors = args.motionVectors;
    params.InJitterOffsetX = args.jitterX;
    params.InJitterOffsetY = args.jitterY;
    params.InRenderSubrectDimensions.Width = m_width;
    params.InRenderSubrectDimensions.Height = m_height;
    params.InReset = args.resetAccumulation;
    params.pInExposureTexture = args.exposure;
    params.pInColorBeforeTransparency = args.colorBeforeTransparency;
    params.pInSpecularHitDistance = args.specularHitDistance;
    params.GBufferSurface.pInAttrib[NVSDK_NGX_GBUFFER_EMISSIVE] = args.emissive;
    params.pInWorldToViewMatrix = const_cast<float*>(&args.device.getGlobalsVS().floatConstants[4][0]);
    params.pInViewToClipMatrix = const_cast<float*>(&args.device.getGlobalsVS().floatConstants[0][0]);
    params.InFrameTimeDeltaInMsec = args.device.getGlobalsPS().floatConstants[68][0] * 1000.0f;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSSD_EXT(
        args.device.getUnderlyingGraphicsCommandList(),
        m_feature, 
        m_ngx.getParameters(),
        &params));
}

UpscalerType DLSSD::getType()
{
    return UpscalerType::DLSSD;
}

bool DLSSD::valid(const NGX& ngx)
{
    int32_t needsUpdatedDriver;
    NVSDK_NGX_Result result = ngx.getParameters()->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_NeedsUpdatedDriver, &needsUpdatedDriver);

    if (NVSDK_NGX_SUCCEED(result) && needsUpdatedDriver != 0)
        return false;

    int32_t available;
    result = ngx.getParameters()->Get(NVSDK_NGX_Parameter_SuperSamplingDenoising_Available, &available);

    if (NVSDK_NGX_FAILED(result) || available == 0)
        return false;

    return true;
}
