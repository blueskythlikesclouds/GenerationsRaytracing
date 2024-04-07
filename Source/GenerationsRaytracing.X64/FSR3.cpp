#include "FSR3.h"
#include "Device.h"

#define THROW_IF_FAILED(x) \
    do \
    { \
        FfxErrorCode errorCode = x; \
        if (errorCode != FFX_OK) \
        { \
            assert(0 && #x); \
        } \
    } while (0)

FSR3::FSR3(const Device& device)
{
    FfxInterface* const backends[] =
    {
        &m_contextDesc.backendInterfaceSharedResources,
        &m_contextDesc.backendInterfaceUpscaling,
        &m_contextDesc.backendInterfaceFrameInterpolation
    };
    static_assert(_countof(backends) == FSR3_BACKEND_COUNT);

    const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR3_CONTEXT_COUNT);
    for (size_t i = 0; i < _countof(backends); i++)
    {
        m_scratchBuffers[i] = std::make_unique<uint8_t[]>(scratchBufferSize);

        THROW_IF_FAILED(ffxGetInterfaceDX12(
            backends[i],
            ffxGetDeviceDX12(device.getUnderlyingDevice()),
            m_scratchBuffers[i].get(),
            scratchBufferSize,
            FFX_FSR3_CONTEXT_COUNT));
    }
}

FSR3::~FSR3()
{
    if (m_valid)
    {
        FfxFrameGenerationConfig config{};
        config.swapChain = ffxGetSwapchainDX12(m_swapChain);
        config.frameGenerationEnabled = false;
        ffxWaitForPresents(config.swapChain);
        ffxFsr3ConfigureFrameGeneration(&m_context, &config);
        ffxFsr3ContextDestroy(&m_context);
    }
}

void FSR3::init(const InitArgs& args)
{
    if (m_valid)
        ffxFsr3ContextDestroy(&m_context);

    m_valid = true;

    if (args.qualityMode == QualityMode::Native)
    {
        m_width = args.width;
        m_height = args.height;
    }
    else
    {
        THROW_IF_FAILED(ffxFsr3GetRenderResolutionFromQualityMode(
            &m_width,
            &m_height,
            args.width,
            args.height,
            args.qualityMode == QualityMode::Quality ? FFX_FSR3_QUALITY_MODE_QUALITY :
            args.qualityMode == QualityMode::Balanced ? FFX_FSR3_QUALITY_MODE_BALANCED :
            args.qualityMode == QualityMode::Performance ? FFX_FSR3_QUALITY_MODE_PERFORMANCE : FFX_FSR3_QUALITY_MODE_ULTRA_PERFORMANCE));
    }

    m_contextDesc.flags = FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE;
    m_contextDesc.maxRenderSize.width = m_width;
    m_contextDesc.maxRenderSize.height = m_height;
    m_contextDesc.upscaleOutputSize.width = args.width;
    m_contextDesc.upscaleOutputSize.height = args.height;
    m_contextDesc.displaySize.width = args.width;
    m_contextDesc.displaySize.height = args.height;
#ifdef _DEBUG
    m_contextDesc.fpMessage = [](auto, const wchar_t* message) { OutputDebugStringW(message); };
#endif
    m_contextDesc.backBufferFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;

    THROW_IF_FAILED(ffxFsr3ContextCreate(&m_context, &m_contextDesc));

    args.device.getSwapChain().replaceSwapChainForFrameInterpolation(args.device);
    m_swapChain = args.device.getSwapChain().getSwapChain();
}

void FSR3::dispatch(const DispatchArgs& args)
{
    FfxFrameGenerationConfig config{};
    config.swapChain = ffxGetSwapchainDX12(args.device.getSwapChain().getSwapChain());
    config.frameGenerationEnabled = true;
    config.frameGenerationCallback = ffxFsr3DispatchFrameGeneration;
    ffxFsr3ConfigureFrameGeneration(&m_context, &config);

    FfxFsr3DispatchUpscaleDescription desc{};
    desc.commandList = ffxGetCommandListDX12(args.device.getUnderlyingGraphicsCommandList());
    desc.color = ffxGetResourceDX12(args.color, GetFfxResourceDescriptionDX12(args.color), nullptr);
    desc.depth = ffxGetResourceDX12(args.depth, GetFfxResourceDescriptionDX12(args.depth), nullptr);
    desc.motionVectors = ffxGetResourceDX12(args.motionVectors, GetFfxResourceDescriptionDX12(args.motionVectors), nullptr);
    desc.exposure = ffxGetResourceDX12(args.exposure, GetFfxResourceDescriptionDX12(args.exposure), nullptr);
    desc.upscaleOutput = ffxGetResourceDX12(args.output, GetFfxResourceDescriptionDX12(args.output), nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    desc.jitterOffset.x = args.jitterX;
    desc.jitterOffset.y = args.jitterY;
    desc.motionVectorScale = { 1.0f, 1.0f };
    desc.renderSize.width = m_width;
    desc.renderSize.height = m_height;
    desc.frameTimeDelta = args.device.getGlobalsPS().floatConstants[68][0] * 1000.0f;
    desc.reset = args.resetAccumulation;
    desc.cameraNear = args.device.getGlobalsPS().floatConstants[25][0];
    desc.cameraFar = args.device.getGlobalsPS().floatConstants[25][1];
    desc.cameraFovAngleVertical = 1.0f / args.device.getGlobalsVS().floatConstants[0][0];
    THROW_IF_FAILED(ffxFsr3ContextDispatchUpscale(&m_context, &desc));
}

UpscalerType FSR3::getType()
{
    return UpscalerType::FSR3;
}
