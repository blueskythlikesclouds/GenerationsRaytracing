#pragma once

class Device;

enum class QualityMode
{
    Native,
    Quality,
    Balanced,
    Performance,
    UltraPerformance
};

enum class UpscalerType
{
    DLSS,
    FSR2
};

class Upscaler
{
protected:
    uint32_t m_width = 0;
    uint32_t m_height = 0;

public:
    struct InitArgs
    {
        Device& device;
        uint32_t width = 0;
        uint32_t height = 0;
        QualityMode qualityMode = QualityMode::Balanced;
    };

    struct DispatchArgs
    {
        Device& device;
        ID3D12Resource* diffuseAlbedo = nullptr;
        ID3D12Resource* specularAlbedo = nullptr;
        ID3D12Resource* normals = nullptr;
        ID3D12Resource* color = nullptr;
        ID3D12Resource* output = nullptr;
        ID3D12Resource* depth = nullptr;
        ID3D12Resource* linearDepth = nullptr;
        ID3D12Resource* motionVectors = nullptr;
        ID3D12Resource* specularHitDistance = nullptr;
        float jitterX = 0.0f;
        float jitterY = 0.0f;
        bool resetAccumulation = false;
    };

    virtual ~Upscaler();
    virtual void init(const InitArgs& args) = 0;
    virtual void dispatch(const DispatchArgs& args) = 0;
    virtual UpscalerType getType() = 0;

    uint32_t getWidth() const;
    uint32_t getHeight() const;
};