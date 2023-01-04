#pragma once

#include "Upscaler.h"

class DLSS : public Upscaler
{
protected:
    NVSDK_NGX_Parameter* parameters = nullptr;
    NVSDK_NGX_Handle* feature = nullptr;

    void validateImp(const ValidationParams& params) override;
    void evaluateImp(const EvaluationParams& params) override;

public:
    DLSS(const Device& device, const std::string& directoryPath);
    ~DLSS() override;
};
