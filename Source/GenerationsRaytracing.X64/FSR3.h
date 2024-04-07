#pragma once

#include "Upscaler.h"

typedef enum Fsr3BackendTypes : uint32_t
{
    FSR3_BACKEND_SHARED_RESOURCES,
    FSR3_BACKEND_UPSCALING,
    FSR3_BACKEND_FRAME_INTERPOLATION,
    FSR3_BACKEND_COUNT
} Fsr3BackendTypes;

class FSR3 : public Upscaler
{
protected:
    IDXGISwapChain4* m_swapChain;
    std::unique_ptr<uint8_t[]> m_scratchBuffers[FSR3_BACKEND_COUNT];
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