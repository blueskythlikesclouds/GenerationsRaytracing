#pragma once

struct Bridge;
struct Device;

struct Upscaler
{
    NVSDK_NGX_Parameter* parameters = nullptr;
    NVSDK_NGX_Handle* feature = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;

    nvrhi::TextureHandle texture;
    nvrhi::TextureHandle depth;
    nvrhi::TextureHandle motionVector;
    nvrhi::TextureHandle output;

    Upscaler(const Device& device, const std::string& directoryPath);
    ~Upscaler();

    void validate(const Bridge& bridge, nvrhi::ITexture* outputTexture);
    void upscale(const Bridge& bridge, float jitterX, float jitterY) const;
};
