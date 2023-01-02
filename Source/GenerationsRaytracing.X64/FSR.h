#pragma once

#include "Upscaler.h"

class FSR : public Upscaler
{
protected:
    std::unique_ptr<uint8_t[]> scratchBuffer;
    FfxFsr2ContextDescription contextDesc{};
    FfxFsr2Context context{};

    void validateImp(const ValidationParams& params) override;
    void evaluateImp(const EvaluationParams& params) override;

public:
    FSR(const Device& device);
    ~FSR();

    uint32_t getJitterPhaseCount() override;
};
