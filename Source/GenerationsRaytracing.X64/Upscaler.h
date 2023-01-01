#pragma once

struct Bridge;
struct Device;

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
        float jitterX = 0.0f;
        float jitterY = 0.0f;
    };

protected:
    virtual void validateImp(const ValidationParams& params) = 0;
    virtual void evaluateImp(const EvaluationParams& params) const = 0;

public:
    uint32_t width = 0;
    uint32_t height = 0;

    nvrhi::TextureHandle texture;
    nvrhi::TextureHandle depth;
    nvrhi::TextureHandle motionVector;
    nvrhi::TextureHandle output;

    virtual ~Upscaler() = default;

    void validate(const ValidationParams& params);
    void evaluate(const EvaluationParams& params) const;
};
