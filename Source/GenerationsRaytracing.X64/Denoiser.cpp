#include "Denoiser.h"

#include "Bridge.h"
#include "Upscaler.h"

void Denoiser::createResource(const Bridge& bridge, nvrhi::ITexture* texture)
{
    auto& resource = resources[texture];

    nri::TextureD3D12Desc desc = {};
    desc.d3d12Resource = texture->getNativeObject(nvrhi::ObjectTypes::D3D12_Resource);
    bridge.device.nriInterface.CreateTextureD3D12(*bridge.device.nriDevice, desc, const_cast<nri::Texture*&>(resource.desc.texture));

    resource.desc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
    resource.desc.nextLayout = nri::TextureLayout::GENERAL;

    static constexpr std::pair<nvrhi::Format, nri::Format> FORMATS[] =
    {
        { nvrhi::Format::R32_FLOAT,    nri::Format::R32_SFLOAT },
        { nvrhi::Format::R8_UNORM,     nri::Format::R8_UNORM },
        { nvrhi::Format::R16_FLOAT,   nri::Format::R16_SFLOAT },
        { nvrhi::Format::RG16_FLOAT,   nri::Format::RG16_SFLOAT },
        { nvrhi::Format::RGBA16_FLOAT, nri::Format::RGBA16_SFLOAT },
        { nvrhi::Format::RGBA16_SNORM, nri::Format::RGBA16_SNORM },
        { nvrhi::Format::RGBA16_UINT,  nri::Format::RGBA16_UINT },
        { nvrhi::Format::RGBA32_FLOAT, nri::Format::RGBA32_SFLOAT },
        { nvrhi::Format::RGBA8_UNORM,  nri::Format::RGBA8_UNORM }
    };

    resource.texture.subresourceStates = &resource.desc;

    for (auto& [nvrhiFormat, nriFormat] : FORMATS)
    {
        if (texture->getDesc().format == nvrhiFormat)
        {
            resource.texture.format = nriFormat;
            break;
        }
    }
}

void Denoiser::init(const Bridge& bridge, const RaytracingBridge& rtBridge)
{
    integration = std::make_unique<NrdIntegration>(1);

    const nrd::MethodDesc methodDescs[] =
    {
        { nrd::Method::REBLUR_DIFFUSE, (uint16_t)rtBridge.upscaler->width, (uint16_t)rtBridge.upscaler->height },
        { nrd::Method::SIGMA_SHADOW, (uint16_t)rtBridge.upscaler->width, (uint16_t)rtBridge.upscaler->height }
    };

    nrd::DenoiserCreationDesc denoiserCreationDesc = {};
    denoiserCreationDesc.requestedMethods = methodDescs;
    denoiserCreationDesc.requestedMethodsNum = _countof(methodDescs);

    bool result = integration->Initialize(denoiserCreationDesc, *bridge.device.nriDevice, bridge.device.nriInterface, bridge.device.nriInterface);
    assert(result);

    nri::CommandBufferD3D12Desc commandBufferDesc{};
    commandBufferDesc.d3d12CommandList = bridge.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_GraphicsCommandList);
    commandBufferDesc.d3d12CommandAllocator = bridge.commandList->getNativeObject(nvrhi::ObjectTypes::D3D12_CommandAllocator);
    bridge.device.nriInterface.CreateCommandBufferD3D12(*bridge.device.nriDevice, commandBufferDesc, commandBuffer);

    createResource(bridge, rtBridge.upscaler->motionVector3D);
    createResource(bridge, rtBridge.upscaler->normal);
    createResource(bridge, rtBridge.upscaler->z);
    createResource(bridge, rtBridge.upscaler->noisyGlobalIllumination);
    createResource(bridge, rtBridge.upscaler->denoisedGlobalIllumination);
    createResource(bridge, rtBridge.upscaler->noisyShadow);
    createResource(bridge, rtBridge.upscaler->denoisedShadow);
}

static void copyMatrix(float* dest, const float* src)
{
#if 0
    memcpy(dest, src, 64);
#else
    for (size_t i = 0; i < 4; i++)
    {
        for (size_t j = 0; j < 4; j++)
            dest[i * 4 + j] = src[j * 4 + i];
    }
#endif
}

void Denoiser::eval(const Bridge& bridge, const RaytracingBridge& rtBridge)
{
    nrd::CommonSettings commonSettings{};

    copyMatrix(commonSettings.viewToClipMatrix, &bridge.vsConstants.c[0][0]);
    copyMatrix(commonSettings.worldToViewMatrix, &bridge.vsConstants.c[4][0]);

    commonSettings.cameraJitter[0] = -rtBridge.rtConstants.jitterX;
    commonSettings.cameraJitter[1] = -rtBridge.rtConstants.jitterY;
    commonSettings.timeDeltaBetweenFrames = bridge.psConstants.c[68][0] * 1000.0f;
    commonSettings.frameIndex = rtBridge.rtConstants.currentFrame;
    commonSettings.isMotionVectorInWorldSpace = true;

    nrd::ReblurSettings reblurSettings{};
    nrd::SigmaSettings sigmaSettings{};

    integration->SetMethodSettings(nrd::Method::REBLUR_DIFFUSE, &reblurSettings);
    integration->SetMethodSettings(nrd::Method::SIGMA_SHADOW, &sigmaSettings);

    NrdUserPool userPool{};
    NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV, resources[rtBridge.upscaler->motionVector3D].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS, resources[rtBridge.upscaler->normal].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_VIEWZ, resources[rtBridge.upscaler->z].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST, resources[rtBridge.upscaler->noisyGlobalIllumination].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SHADOWDATA, resources[rtBridge.upscaler->noisyShadow].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST, resources[rtBridge.upscaler->denoisedGlobalIllumination].texture);
    NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SHADOW_TRANSLUCENCY, resources[rtBridge.upscaler->denoisedShadow].texture);

    integration->Denoise(rtBridge.rtConstants.currentFrame, *commandBuffer, commonSettings, userPool, false);
    bridge.commandList->clearState();
}
