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

Upscaler::Upscaler()
{
    const INIReader reader("GenerationsRaytracing.ini");
    qualityMode = (QualityMode)reader.GetInteger("Mod", "QualityMode", 0);
}

// https://github.com/DarioSamo/RT64/blob/main/src/rt64lib/private/rt64_common.h
static float haltonSequence(int i, int b)
{
    float f = 1.0;
    float r = 0.0;

    while (i > 0)
    {
        f = f / float(b);
        r = r + f * float(i % b);
        i = i / b;
    }

    return r;
}

static void haltonJitter(int frame, int phases, float& jitterX, float& jitterY)
{
    jitterX = haltonSequence(frame % phases + 1, 2) - 0.5f;
    jitterY = haltonSequence(frame % phases + 1, 3) - 0.5f;
}

void Upscaler::getJitterOffset(size_t currentFrame, float& jitterX, float& jitterY)
{
    haltonJitter((int)currentFrame, getJitterPhaseCount(), jitterX, jitterY);
}

uint32_t Upscaler::getJitterPhaseCount()
{
    return 64;
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

    position = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA32_FLOAT));
    depth = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::R32_FLOAT));
    motionVector = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RG16_FLOAT));
    normal = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_SNORM));
    texCoord = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RG16_FLOAT));
    color = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA8_UNORM));
    shader = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_UINT));

    globalIllumination = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_FLOAT));
    shadow = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::R8_UNORM));
    reflection = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_FLOAT));
    refraction = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA16_FLOAT));

    composite = params.bridge.device.nvrhi->createTexture(textureDesc.setFormat(nvrhi::Format::RGBA32_FLOAT));

    output = params.output;
}

void Upscaler::evaluate(const EvaluationParams& params)
{
    params.bridge.commandList->setTextureState(composite, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(depth, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(motionVector, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::ShaderResource);
    params.bridge.commandList->setTextureState(output, nvrhi::TextureSubresourceSet(), nvrhi::ResourceStates::UnorderedAccess);
    params.bridge.commandList->commitBarriers();

    evaluateImp(params);
}
