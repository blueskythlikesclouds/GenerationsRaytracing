#pragma once

#include "ShaderMapping.h"

struct App;

struct ShaderLibrary
{
    std::unique_ptr<uint8_t[]> byteCode;
    size_t byteSize = 0;
    ShaderMapping shaderMapping;

    nvrhi::ShaderLibraryHandle handle;
    nvrhi::ShaderHandle copyVS;
    nvrhi::ShaderHandle copyPS;
    nvrhi::ShaderHandle accumulatePS;
    nvrhi::ShaderHandle copyLuminancePS;
    nvrhi::ShaderHandle lerpLuminancePS;
    nvrhi::ShaderHandle toneMapPS;

    ShaderLibrary(const App& app, const std::string& directoryPath);
};
