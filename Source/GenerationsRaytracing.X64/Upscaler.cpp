#include "Upscaler.h"

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

Upscaler::Upscaler(const Device& device, const std::string& directoryPath)
{
    constexpr char APPLICATION_ID[8] = "GensRT";

    THROW_IF_FAILED(NVSDK_NGX_D3D12_Init(*(unsigned long long*) &APPLICATION_ID[0], multiByteToWideChar(directoryPath.c_str()).c_str(), device.d3d12.device.Get()));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&parameters));
}

Upscaler::~Upscaler()
{
    THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(feature));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(parameters));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown());
}

void Upscaler::validate(const Bridge& bridge, nvrhi::ITexture* outputTexture)
{
    if (outputTexture == output)
        return;

    unsigned tmpUnsigned;
    float tmpFloat;

    THROW_IF_FAILED(NGX_DLSS_GET_OPTIMAL_SETTINGS(
        parameters,
        outputTexture->getDesc().width,
        outputTexture->getDesc().height,
        NVSDK_NGX_PerfQuality_Value_MaxPerf,
        &width,
        &height,
        &tmpUnsigned,
        &tmpUnsigned,
        &tmpUnsigned,
        &tmpUnsigned,
        &tmpFloat));

    assert(width > 0 && height > 0);

    NVSDK_NGX_DLSS_Create_Params params{};
    params.Feature.InWidth = width;
    params.Feature.InHeight = height;
    params.Feature.InTargetWidth = outputTexture->getDesc().width;
    params.Feature.InTargetHeight = outputTexture->getDesc().height;
    params.Feature.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_MaxPerf;
    params.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes;

    auto commandList = bridge.device.nvrhi->createCommandList(nvrhi::CommandListParameters()
        .setEnableImmediateExecution(false));

    commandList->open();

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSS_EXT(
        commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList),
        1,
        1,
        &feature,
        parameters,
        &params));

    commandList->close();

    bridge.device.nvrhi->executeCommandList(commandList);

    auto textureDesc = nvrhi::TextureDesc()
        .setIsUAV(true)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true);

    texture = bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    depth = bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::R32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    motionVector = bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RG16_FLOAT)
        .setWidth(width)
        .setHeight(height));

    output = outputTexture;
    bridge.device.nvrhi->waitForIdle();
}

void Upscaler::upscale(const Bridge& bridge, float jitterX, float jitterY) const
{
    bridge.commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    bridge.commandList->setTextureState(depth, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    bridge.commandList->setTextureState(motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    bridge.commandList->setTextureState(output, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    bridge.commandList->commitBarriers();

    NVSDK_NGX_D3D12_DLSS_Eval_Params params{};

    params.Feature.pInColor = texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.Feature.pInOutput = output->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.pInDepth = depth->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.pInMotionVectors = motionVector->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.InJitterOffsetX = -jitterX;
    params.InJitterOffsetY = -jitterY;
    params.InRenderSubrectDimensions.Width = width;
    params.InRenderSubrectDimensions.Height = height;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSS_EXT(bridge.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList), feature, parameters, &params));

    bridge.commandList->clearState();
}
