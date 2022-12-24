#pragma once

struct Device
{
    struct
    {
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> graphicsCommandQueue;
    } d3d12;

    ComPtr<IDXGIFactory4> dxgiFactory;
    nvrhi::DeviceHandle nvrhi;

    Device();
    ~Device();
};