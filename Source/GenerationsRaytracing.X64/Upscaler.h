#pragma once

struct Bridge;
struct Device;

enum class QualityMode
{
    Native,
    Quality,
    Balanced,
    Performance,
    UltraPerformance
};

class Upscaler
{
public:
    struct ValidationParams
    {
        const Bridge& bridge;
        nvrhi::ITexture* output = nullptr;
    };

    struct EvaluationParams
    {
        const Bridge& bridge;
        size_t currentFrame = 0;
        float jitterX = 0.0f;
        float jitterY = 0.0f;
    };

protected:
    virtual void validateImp(const ValidationParams& params) = 0;
    virtual void evaluateImp(const EvaluationParams& params) = 0;

public:
    QualityMode qualityMode{};
    uint32_t width = 0;
    uint32_t height = 0;

    struct PingPongTexture
    {
        nvrhi::TextureHandle ping;
        nvrhi::TextureHandle pong;

        void create(nvrhi::IDevice* device, const nvrhi::TextureDesc& desc);

        nvrhi::ITexture* getCurrent(size_t frameIndex) const;
        nvrhi::ITexture* getPrevious(size_t frameIndex) const;
    };

    nvrhi::TextureHandle color;
    PingPongTexture depth;
    nvrhi::TextureHandle motionVector;

    PingPongTexture normal;
    PingPongTexture globalIllumination;

    nvrhi::TextureHandle output;

    Upscaler();
    virtual ~Upscaler() = default;

    virtual void getJitterOffset(size_t currentFrame, float& jitterX, float& jitterY);
    virtual uint32_t getJitterPhaseCount();

    void validate(const ValidationParams& params);
    void evaluate(const EvaluationParams& params);
};
