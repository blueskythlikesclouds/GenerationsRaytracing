#include "FSR2.h"
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

FSR2::FSR2(const Device& device)
{
    const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
    m_scratchBuffer = std::make_unique<uint8_t[]>(scratchBufferSize);

    THROW_IF_FAILED(ffxFsr2GetInterfaceDX12(&m_contextDesc.callbacks, device.getUnderlyingDevice(), m_scratchBuffer.get(), scratchBufferSize));
}

FSR2::~FSR2()
{
    if (m_contextDesc.device != nullptr)
        ffxFsr2ContextDestroy(&m_context);
}

void FSR2::init(const InitArgs& args)
{
    if (m_contextDesc.device != nullptr)
        ffxFsr2ContextDestroy(&m_context);

    if (args.qualityMode == QualityMode::Native)
    {
        m_width = args.width;
        m_height = args.height;
    }
    else
    {
        THROW_IF_FAILED(ffxFsr2GetRenderResolutionFromQualityMode(
            &m_width,
            &m_height,
            args.width,
            args.height,
            args.qualityMode == QualityMode::Quality ? FFX_FSR2_QUALITY_MODE_QUALITY :
            args.qualityMode == QualityMode::Balanced ? FFX_FSR2_QUALITY_MODE_BALANCED :
            args.qualityMode == QualityMode::Performance ? FFX_FSR2_QUALITY_MODE_PERFORMANCE : FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE));
    }

    m_contextDesc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
    m_contextDesc.maxRenderSize.width = m_width;
    m_contextDesc.maxRenderSize.height = m_height;
    m_contextDesc.displaySize.width = args.width;
    m_contextDesc.displaySize.height = args.height;
    m_contextDesc.device = ffxGetDeviceDX12(args.device.getUnderlyingDevice());

    THROW_IF_FAILED(ffxFsr2ContextCreate(&m_context, &m_contextDesc));
}

void FSR2::dispatch(const DispatchArgs& args)
{
    FfxFsr2DispatchDescription desc{};
    desc.commandList = ffxGetCommandListDX12(args.device.getUnderlyingGraphicsCommandList());
    desc.color = ffxGetResourceDX12(&m_context, args.color);
    desc.depth = ffxGetResourceDX12(&m_context, args.depth);
    desc.motionVectors = ffxGetResourceDX12(&m_context, args.motionVectors);
    desc.output = ffxGetResourceDX12(&m_context, args.output, nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);
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
    THROW_IF_FAILED(ffxFsr2ContextDispatch(&m_context, &desc));
}

UpscalerType FSR2::getType()
{
    return UpscalerType::FSR2;
}
