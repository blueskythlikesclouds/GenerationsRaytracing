#pragma once

struct App;

struct DLSSPass
{
    NVSDK_NGX_Parameter* parameters = nullptr;
    NVSDK_NGX_Handle* feature = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    float sharpness = 0.0f;

    nvrhi::TextureHandle texture;
    nvrhi::TextureHandle depthTexture;
    nvrhi::TextureHandle motionVectorTexture;
    nvrhi::TextureHandle outputTexture;

    DLSSPass(const App& app, const std::string& directoryPath);
    ~DLSSPass();

    void execute(const App& app);
};
