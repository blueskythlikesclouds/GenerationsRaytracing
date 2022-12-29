#pragma once

class MemoryAllocator;

struct Device
{
    struct
    {
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> graphicsCommandQueue;
    } d3d12;

    ComPtr<IDXGIFactory4> dxgiFactory;

    std::unique_ptr<MemoryAllocator> memoryAllocator;
    nvrhi::DeviceHandle nvrhi;

    Device();
    ~Device();
};