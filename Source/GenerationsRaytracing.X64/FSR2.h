#pragma once

#include "Upscaler.h"

class FSR2 : public Upscaler
{
protected:
    std::unique_ptr<uint8_t[]> m_scratchBuffer;
    FfxFsr2ContextDescription m_contextDesc{};
    FfxFsr2Context m_context{};

public:
    FSR2(const Device& device);
    ~FSR2() override;
    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;
    UpscalerType getType() override;
};