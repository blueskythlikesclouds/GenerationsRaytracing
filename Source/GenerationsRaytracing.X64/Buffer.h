#pragma once

struct Buffer
{
    ComPtr<D3D12MA::Allocation> allocation;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t byteSize = 0;
};