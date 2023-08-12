#pragma once

struct Texture
{
    ComPtr<ID3D12Resource> resource;
    uint32_t srvIndex = 0;
    uint32_t rtvIndex = 0;
    uint32_t dsvIndex = 0;
    // 4 bytes wasted...
};