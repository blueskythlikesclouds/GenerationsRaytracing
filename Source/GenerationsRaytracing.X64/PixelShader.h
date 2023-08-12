#pragma once

struct PixelShader
{
    std::unique_ptr<uint8_t[]> byteCode;
    uint32_t byteSize = 0;
    // 4 bytes wasted...
};
