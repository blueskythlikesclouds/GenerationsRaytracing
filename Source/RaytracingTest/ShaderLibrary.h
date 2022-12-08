#pragma once

#include "ShaderMapping.h"

struct ShaderLibrary
{
    std::unique_ptr<uint8_t[]> byteCode;
    size_t byteSize = 0;
    ShaderMapping shaderMapping;

    ShaderLibrary(const std::string& directoryPath);
};
