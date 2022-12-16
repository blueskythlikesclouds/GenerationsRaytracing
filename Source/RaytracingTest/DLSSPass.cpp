#include "DLSSPass.h"

#include "App.h"
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

DLSSPass::DLSSPass(const App& app, const std::string& directoryPath)
{
    constexpr char APPLICATION_ID[8] = "GENSRTX";
    unsigned int _;

    THROW_IF_FAILED(NVSDK_NGX_D3D12_Init(*(unsigned long long*)&APPLICATION_ID[0], toNchar(directoryPath.c_str()).data(), app.device.d3d12.device.Get()));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&parameters));

    THROW_IF_FAILED(NGX_DLSS_GET_OPTIMAL_SETTINGS(
        parameters,
        app.width,
        app.height,
        NVSDK_NGX_PerfQuality_Value_Balanced,
        &width,
        &height,
        &_,
        &_,
        &_,
        &_,
        &sharpness));

    assert(width > 0 && height > 0);

    NVSDK_NGX_DLSS_Create_Params params {};
    params.Feature.InWidth = width;
    params.Feature.InHeight = height;
    params.Feature.InTargetWidth = app.width;
    params.Feature.InTargetHeight = app.height;
    params.Feature.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_Balanced;
    params.InFeatureCreateFlags =
        NVSDK_NGX_DLSS_Feature_Flags_IsHDR |
        NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
        NVSDK_NGX_DLSS_Feature_Flags_DoSharpening |
        NVSDK_NGX_DLSS_Feature_Flags_AutoExposure;

    app.renderer.commandList->open();

    THROW_IF_FAILED(NGX_D3D12_CREATE_DLSS_EXT(
        app.renderer.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList),
        1,
        1,
        &feature,
        parameters,
        &params));

    app.renderer.commandList->close();

    app.device.nvrhi->executeCommandList(app.renderer.commandList);
    app.device.nvrhi->waitForIdle();

    auto textureDesc = nvrhi::TextureDesc()
        .setIsUAV(true)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true);

    texture = app.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    depthTexture = app.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::R32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    motionVectorTexture = app.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RG16_FLOAT)
        .setWidth(width)
        .setHeight(height));

    outputTexture = app.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setWidth(app.width)
        .setHeight(app.height));
}

DLSSPass::~DLSSPass()
{
    THROW_IF_FAILED(NVSDK_NGX_D3D12_ReleaseFeature(feature));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(parameters));
    THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown());
}

void DLSSPass::execute(const App& app)
{
    app.renderer.commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    app.renderer.commandList->setTextureState(depthTexture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    app.renderer.commandList->setTextureState(motionVectorTexture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    app.renderer.commandList->setTextureState(outputTexture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    app.renderer.commandList->commitBarriers();

    NVSDK_NGX_D3D12_DLSS_Eval_Params params {};

    params.Feature.pInColor = texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.Feature.pInOutput = outputTexture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.Feature.InSharpness = sharpness;
    params.pInDepth = depthTexture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.pInMotionVectors = motionVectorTexture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    params.InJitterOffsetX = -app.camera.pixelJitter.x();
    params.InJitterOffsetY = -app.camera.pixelJitter.y();
    params.InRenderSubrectDimensions.Width = width;
    params.InRenderSubrectDimensions.Height = height;
    params.InFrameTimeDeltaInMsec = app.deltaTime * 1000.0f;

    THROW_IF_FAILED(NGX_D3D12_EVALUATE_DLSS_EXT(app.renderer.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList), feature, parameters, &params));

    app.renderer.commandList->clearState();
}
