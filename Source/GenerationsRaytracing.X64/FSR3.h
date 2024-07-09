#pragma once

#include "Upscaler.h"

class FSR3 : public Upscaler
{
protected:
    ffx::Context m_context{};

public:
    FSR3(const Device& device);
    ~FSR3() override;
    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;
    UpscalerType getType() override;
};