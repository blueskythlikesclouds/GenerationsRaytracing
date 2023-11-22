#pragma once

struct Buffer
{
    ComPtr<D3D12MA::Allocation> allocation;
    ComPtr<D3D12MA::Allocation> nextAllocation;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t byteSize = 0;
    uint32_t srvIndex = 0;
};