#pragma once

#include "Upscaler.h"

class FSR3 : public Upscaler
{
protected:
    std::unique_ptr<uint8_t[]> m_scratchBuffer;
    FfxFsr3ContextDescription m_contextDesc{};
    FfxFsr3Context m_context{};
    bool m_valid = false;

public:
    FSR3(const Device& device);
    ~FSR3() override;
    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;
    UpscalerType getType() override;
};