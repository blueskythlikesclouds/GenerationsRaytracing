#pragma once

class Device;

class FrameGenerator
{
protected:
    ffx::Context m_context{};

public:
    struct InitArgs
    {
        Device& device;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        bool hdr = false;
    };

    struct DispatchArgs
    {
        Device& device;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t currentFrame = 0;
        uint32_t renderWidth = 0;
        uint32_t renderHeight = 0;
        ID3D12Resource* depth = nullptr;
        ID3D12Resource* motionVectors = nullptr;
        float jitterX = 0.0f;
        float jitterY = 0.0f;
        bool resetAccumulation = false;
    };

    void init(const InitArgs& args);
    void dispatch(const DispatchArgs& args);
};