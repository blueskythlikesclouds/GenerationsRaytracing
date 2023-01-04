#pragma once

struct RaytracingBridge;
struct Bridge;

struct Denoiser
{
    struct Resource
    {
        nri::TextureTransitionBarrierDesc desc{};
        NrdIntegrationTexture texture{};
    };

    std::unique_ptr<NrdIntegration> integration;
    nri::CommandBuffer* commandBuffer = nullptr;

    std::unordered_map<nvrhi::ITexture*, Resource> resources;

    void createResource(const Bridge& bridge, nvrhi::ITexture* texture);

    void init(const Bridge& bridge, const RaytracingBridge& rtBridge);
    void eval(const Bridge& bridge, const RaytracingBridge& rtBridge);
};
