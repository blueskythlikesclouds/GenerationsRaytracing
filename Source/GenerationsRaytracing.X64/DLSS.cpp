#include "DLSS.h"

#include "Bridge.h"
#include "Device.h"
#include "Path.h"

#define THROW_IF_FAILED(x) \
    do \
    { \
        const NVSDK_NGX_Result result = x; \
        if (NVSDK_NGX_FAILED(result)) \
        { \
            printf("code = %x, string = %ls\n", result, GetNGXResultAsString(result)); \
            assert(0 && x); \
        } \
    } while (0)

void DLSS::validateImp(const ValidationParams& params)
{
    if (qualityMode == QualityMode::Native)
    {
        width = params.output->getDesc().width;
        height = params.output->getDesc().height;
    }
    else
    {
        unsigned tmpUnsigned;
        float tmpFloat;

        THROW_IF_FAILED(NGX_DLSS_GET_OPTIMAL_SETTINGS(
            parameters,
            params.output->getDesc().width,
            params.output->getDesc().height,
            qualityMode == QualityMode::Quality ? NVSDK_NGX_PerfQuality_Value_MaxQuality :
            qualityMode == QualityMode::Balanced ? NVSDK_NGX_PerfQuality_Value_Balanced :
            qualityMode == QualityMode::Performance ? NVSDK_NGX_PerfQuality_Value_MaxPerf :
            qualityMode == QualityMode::UltraPerformance ? NVSDK_NGX_PerfQuality_Value_UltraPerformance : NVSDK_NGX_PerfQuality_Value_UltraQuality,
            &width,
            &height,
            &tmpUnsigned,
            &tmpUnsigned,
            &tmpUnsigned,
            &tmpUnsigned,
            &tmpFloat));

        assert(width > 0 && height > 0);
    }

    NVSDK_NGX_DLSS_Create_Params dlssParams{};
    dlssParams.Feature.InWidth = width;
    dlssParams.Feature.InHeight = height;
    dlssParams.Feature.InTargetWidth = params.output->getDesc().width;
    dlssParams.Feature.InTargetHeight = params.output->getDesc().height;
    dlssParams.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

    auto commandList = params.bridge.device.nvrhi->createCommandList(nvrhi::CommandListParameters()
        .setEnableImmediateExecution(false));

    commandList->open();

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSS_EXT(
        commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList),
        1,
        1,
        &feature,
        parameters,
        &dlssParams));

    commandList->close();

    params.bridge.device.nvrhi->executeCommandList(commandList);
    params.bridge.device.nvrhi->waitForIdle();
}

void DLSS::evaluateImp(const EvaluationParams& params)
{
    NVSDK_NGX_D3D12_DLSS_Eval_Params dlssParams{};

    dlssParams.Feature.pInColor = composite->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    dlssParams.Feature.pInOutput = output->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    dlssParams.pInDepth = depth->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    dlssParams.pInMotionVectors = motionVector->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    dlssParams.InJitterOffsetX = params.jitterX;
    dlssParams.InJitterOffsetY = params.jitterY;
    dlssParams.InRenderSubrectDimensions.Width = width;
    dlssParams.InRenderSubrectDimensions.Height = height;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSS_EXT(params.bridge.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList), feature, parameters, &dlssParams));
    params.bridge.commandList->clearState();
}

DLSS::DLSS(const Device& device, const std::string& directoryPath) : Upscaler()
{
    constexpr char APPLICATION_ID[8] = "GensRT";

    THROW_IF_FAILED(NVSDK_NGX_D3D12_Init(*(unsigned long long*) & APPLICATION_ID[0], multiByteToWideChar(directoryPath.c_str()).c_str(), device.d3d12.device.Get()));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&parameters));
}

DLSS::~DLSS()
{
    THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(feature));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(parameters));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown());
}