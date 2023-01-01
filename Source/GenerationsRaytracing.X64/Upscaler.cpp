#include "Upscaler.h"

#include "Bridge.h"
#include "Device.h"

void Upscaler::validate(const ValidationParams& params)
{
    if (params.output == output)
        return;

    validateImp(params);

    auto textureDesc = nvrhi::TextureDesc()
        .setIsUAV(true)
        .setInitialState(nvrhi::ResourceStates::UnorderedAccess)
        .setKeepInitialState(true);

    texture = params.bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RGBA32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    depth = params.bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::R32_FLOAT)
        .setWidth(width)
        .setHeight(height));

    motionVector = params.bridge.device.nvrhi->createTexture(textureDesc
        .setFormat(nvrhi::Format::RG16_FLOAT)
        .setWidth(width)
        .setHeight(height));

    output = params.output;
}

void Upscaler::evaluate(const EvaluationParams& params) const
{
    params.bridge.commandList->setTextureState(texture, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(depth, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(output, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    params.bridge.commandList->commitBarriers();

    evaluateImp(params);
}
