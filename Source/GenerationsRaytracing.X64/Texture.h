#pragma once

struct Texture
{
    ComPtr<D3D12MA::Allocation> allocation;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    bool isShadowMap = false;
    uint32_t srvIndex = 0;
    uint32_t rtvIndex = 0;
    uint32_t dsvIndex = 0;
};