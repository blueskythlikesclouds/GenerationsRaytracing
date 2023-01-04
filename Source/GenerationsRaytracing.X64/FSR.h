#pragma once

#include "Upscaler.h"

class FSR : public Upscaler
{
protected:
    std::unique_ptr<uint8_t[]> scratchBuffer;
    FfxFsr2ContextDescription contextDesc{};
    FfxFsr2Context context{};

    void validateImp(const InitParams& params) override;
    void evaluateImp(const EvalParams& params) override;

public:
    FSR(const Device& device);
    ~FSR();

    void getJitterOffset(size_t currentFrame, float& jitterX, float& jitterY) override;
    uint32_t getJitterPhaseCount() override;
};
