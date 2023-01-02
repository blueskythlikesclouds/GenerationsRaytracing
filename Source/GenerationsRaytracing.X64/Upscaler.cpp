#include "Upscaler.h"

#include "Bridge.h"
#include "Device.h"

void Upscaler::PingPongTexture::create(nvrhi::IDevice* device, const nvrhi::TextureDesc& desc)
{
    ping = device->createTexture(desc);
    pong = device->createTexture(desc);
}

nvrhi::ITexture* Upscaler::PingPongTexture::getCurrent(size_t frameIndex) const
{
    return (frameIndex & 1) == 0 ? ping : pong;
}

nvrhi::ITexture* Upscaler::PingPongTexture::getPrevious(size_t frameIndex) const
{
    return (frameIndex & 1) == 0 ? pong : ping;
}

void Upscaler::validate(const ValidationParams& params)
{
    if (params.output == output)
        return;

    validateImp(params);

    auto textureDesc = nvrhi::TextureDesc()
        .setWidth(width)
        .setHeight(height)
        .setIsUAV(true)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true);

    color = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_FLOAT));
    depth.create(params.bridge.device.nvrhi, textureDesc.setFormat(nvrhi::Format::R32_FLOAT));  
    motionVector = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RG16_FLOAT));

    normal.create(params.bridge.device.nvrhi, textureDesc.setFormat(nvrhi::Format::RGBA8_SNORM));
    globalIllumination.create(params.bridge.device.nvrhi, textureDesc.setFormat(nvrhi::Format::RGBA32_FLOAT));

    output = params.output;
}

void Upscaler::evaluate(const EvaluationParams& params)
{
    params.bridge.commandList->setTextureState(color, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(depth.getCurrent(params.currentFrame), nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(output, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    params.bridge.commandList->commitBarriers();

    evaluateImp(params);
}
