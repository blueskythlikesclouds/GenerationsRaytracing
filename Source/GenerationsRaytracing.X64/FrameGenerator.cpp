#include "FrameGenerator.h"

#include "Device.h"
#include "FFX.h"

void FrameGenerator::init(const InitArgs& args)
{
    args.device.getSwapChain().replaceSwapChainForFrameInterpolation(args.device);

    if (m_context != nullptr)
        FFX_THROW_IF_FAILED(ffx::DestroyContext(m_context));

    auto desc = makeFfxObject<ffx::CreateContextDescFrameGeneration>();
    
    if (args.hdr)
        desc.flags = FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE;
    
    desc.displaySize.width = args.width;
    desc.displaySize.height = args.height;
    desc.maxRenderSize.width = args.renderWidth;
    desc.maxRenderSize.height = args.renderHeight;
    desc.backBufferFormat = ffxApiGetSurfaceFormatDX12(args.device.getSwapChain().getResource()->GetDesc().Format);

    auto backendDesc = makeFfxObject<ffx::CreateBackendDX12Desc>();
    backendDesc.device = args.device.getUnderlyingDevice();

    FFX_THROW_IF_FAILED(ffx::CreateContext(m_context, nullptr, desc, backendDesc));
}

void FrameGenerator::dispatch(const DispatchArgs& args)
{
    auto configDesc = makeFfxObject<ffx::ConfigureDescFrameGeneration>();

    configDesc.swapChain = args.device.getSwapChain().getUnderlyingSwapChain();
    configDesc.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
        {
            return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
        };
    configDesc.frameGenerationCallbackUserContext = &m_context;
    configDesc.frameGenerationEnabled = true;
    configDesc.generationRect.width = args.width;
    configDesc.generationRect.height = args.height;
    configDesc.frameID = args.currentFrame;

    FFX_THROW_IF_FAILED(ffx::Configure(m_context, configDesc));

    auto desc = makeFfxObject<ffx::DispatchDescFrameGenerationPrepare>();

    desc.frameID = args.currentFrame;
    desc.commandList = args.device.getUnderlyingGraphicsCommandList();
    desc.renderSize.width = args.renderWidth;
    desc.renderSize.height = args.renderHeight;
    desc.jitterOffset.x = args.jitterX;
    desc.jitterOffset.y = args.jitterY;
    desc.motionVectorScale = { 1.0f, 1.0f };
    desc.frameTimeDelta = args.device.getGlobalsPS().floatConstants[68][0] * 1000.0f;
    desc.unused_reset = args.resetAccumulation;
    desc.cameraNear = args.device.getGlobalsPS().floatConstants[25][0];
    desc.cameraFar = args.device.getGlobalsPS().floatConstants[25][1];
    desc.cameraFovAngleVertical = 1.0f / args.device.getGlobalsVS().floatConstants[0][0];
    desc.viewSpaceToMetersFactor = 1.0f;
    desc.depth = ffxApiGetResourceDX12(args.depth);
    desc.motionVectors = ffxApiGetResourceDX12(args.motionVectors);

    FFX_THROW_IF_FAILED(ffx::Dispatch(m_context, desc));
}
