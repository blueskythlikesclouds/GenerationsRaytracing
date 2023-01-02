#include "FSR.h"

#include "Bridge.h"
#include "Device.h"

#define THROW_IF_FAILED(x) \
    do \
    { \
        FfxErrorCode errorCode = x; \
        if (errorCode != FFX_OK) \
        { \
            printf(#x " failed with %x error code\n", errorCode); \
            assert(0 && #x); \
        } \
    } while (0)

void FSR::validateImp(const ValidationParams& params)
{
    THROW_IF_FAILED(ffxFsr2GetRenderResolutionFromQualityMode(
        &width, 
        &height, 
        params.output->getDesc().width, 
        params.output->getDesc().height,
        qualityMode == QualityMode::Quality ? FFX_FSR2_QUALITY_MODE_QUALITY :
        qualityMode == QualityMode::Balanced ? FFX_FSR2_QUALITY_MODE_BALANCED :
        qualityMode == QualityMode::Performance ? FFX_FSR2_QUALITY_MODE_PERFORMANCE : FFX_FSR2_QUALITY_MODE_ULTRA_PERFORMANCE));

    contextDesc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE;
    contextDesc.maxRenderSize.width = width;
    contextDesc.maxRenderSize.height = height;
    contextDesc.displaySize.width = params.output->getDesc().width;
    contextDesc.displaySize.height = params.output->getDesc().height;
    contextDesc.device = ffxGetDeviceDX12(params.bridge.device.d3d12.device.Get());

    THROW_IF_FAILED(ffxFsr2ContextCreate(&context, &contextDesc));
}

void FSR::evaluateImp(const EvaluationParams& params)
{
    FfxFsr2DispatchDescription desc{};
    desc.commandList = ffxGetCommandListDX12(params.bridge.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList));
    desc.color = ffxGetResourceDX12(&context, color->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource), L"color");
    desc.depth = ffxGetResourceDX12(&context, depth.getCurrent(params.currentFrame)->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource), L"depth");
    desc.motionVectors = ffxGetResourceDX12(&context, motionVector->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource), L"motionVector");
    desc.output = ffxGetResourceDX12(&context, output->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource), L"output", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
    desc.jitterOffset.x = params.jitterX;
    desc.jitterOffset.y = params.jitterY;
    desc.motionVectorScale = { 1.0f, 1.0f };
    desc.renderSize.width = width;
    desc.renderSize.height = height;
    desc.frameTimeDelta = params.bridge.psConstants.c[68][0] * 1000.0f;
    desc.cameraNear = params.bridge.psConstants.c[25][0];
    desc.cameraFar = params.bridge.psConstants.c[25][1];
    desc.cameraFovAngleVertical = 1.0f / params.bridge.vsConstants.c[0][0];
    THROW_IF_FAILED(ffxFsr2ContextDispatch(&context, &desc));

    params.bridge.commandList->clearState();
}

FSR::~FSR()
{
    ffxFsr2ContextDestroy(&context);
}

void FSR::getJitterOffset(size_t currentFrame, float& jitterX, float& jitterY)
{
    ffxFsr2GetJitterOffset(&jitterX, &jitterY, (int32_t)currentFrame, (int32_t)getJitterPhaseCount());
}

FSR::FSR(const Device& device) : Upscaler()
{
    const size_t scratchBufferSize = ffxFsr2GetScratchMemorySizeDX12();
    scratchBuffer = std::make_unique<uint8_t[]>(scratchBufferSize);

    THROW_IF_FAILED(ffxFsr2GetInterfaceDX12(&contextDesc.callbacks, device.d3d12.device.Get(), scratchBuffer.get(), scratchBufferSize));
}

uint32_t FSR::getJitterPhaseCount()
{
    return ffxFsr2GetJitterPhaseCount(width, output->getDesc().width);
}
