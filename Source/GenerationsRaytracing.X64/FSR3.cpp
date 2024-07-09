#include "FSR3.h"
#include "Device.h"

#define THROW_IF_FAILED(x) \
    do \
    { \
        ffx::ReturnCode returnCode = x; \
        if (returnCode != ffx::ReturnCode::Ok) \
        { \
            assert(0 && #x); \
        } \
    } while (0)

template<typename T>
static T makeFfxObject()
{
    alignas(alignof(T)) uint8_t bytes[sizeof(T)]{};
    new (bytes) T();
    return *reinterpret_cast<T*>(bytes);
}

FSR3::FSR3(const Device& device)
{
}

FSR3::~FSR3()
{
    if (m_context != nullptr)
        ffx::DestroyContext(m_context);
}

void FSR3::init(const InitArgs& args)
{
    if (m_context != nullptr)
    {
        ffx::DestroyContext(m_context);
        m_context = nullptr;
    }

    auto query = makeFfxObject<ffx::QueryDescUpscaleGetRenderResolutionFromQualityMode>();
    query.displayWidth = args.width;
    query.displayHeight = args.height;
    query.qualityMode = 
        args.qualityMode == QualityMode::Native ? FFX_UPSCALE_QUALITY_MODE_NATIVEAA :
        args.qualityMode == QualityMode::Quality ? FFX_UPSCALE_QUALITY_MODE_QUALITY :
        args.qualityMode == QualityMode::Balanced ? FFX_UPSCALE_QUALITY_MODE_BALANCED :
        args.qualityMode == QualityMode::Performance ? FFX_UPSCALE_QUALITY_MODE_PERFORMANCE :
        FFX_UPSCALE_QUALITY_MODE_ULTRA_PERFORMANCE;
    query.pOutRenderWidth = &m_width;
    query.pOutRenderHeight = &m_height;
    
    THROW_IF_FAILED(ffx::Query(query));

    auto desc = makeFfxObject<ffx::CreateContextDescUpscale>();
    desc.flags = FFX_UPSCALE_ENABLE_HIGH_DYNAMIC_RANGE;
    desc.maxRenderSize.width = m_width;
    desc.maxRenderSize.height = m_height;
    desc.maxUpscaleSize.width = args.width;
    desc.maxUpscaleSize.height = args.height;
#ifdef _DEBUG
    desc.fpMessage = [](auto, const wchar_t* message) { OutputDebugStringW(message); };
#endif

    auto backendDesc = makeFfxObject<ffx::CreateBackendDX12Desc>();
    backendDesc.device = args.device.getUnderlyingDevice();

    THROW_IF_FAILED(ffx::CreateContext(m_context, nullptr, desc, backendDesc));
}

void FSR3::dispatch(const DispatchArgs& args)
{
    auto desc = makeFfxObject<ffx::DispatchDescUpscale>();

    desc.commandList = args.device.getUnderlyingGraphicsCommandList();
    desc.color = ffxApiGetResourceDX12(args.color);
    desc.depth = ffxApiGetResourceDX12(args.depth);
    desc.motionVectors = ffxApiGetResourceDX12(args.motionVectors);
    desc.exposure = ffxApiGetResourceDX12(args.exposure);
    desc.output = ffxApiGetResourceDX12(args.output, FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
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

    THROW_IF_FAILED(ffx::Dispatch(m_context, desc));
}

UpscalerType FSR3::getType()
{
    return UpscalerType::FSR3;
}
