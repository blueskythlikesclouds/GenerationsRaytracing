#pragma once

struct Window;

struct Device
{
    struct
    {
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> graphicsCommandQueue;
        ComPtr<ID3D12CommandQueue> computeCommandQueue;
        ComPtr<ID3D12CommandQueue> copyCommandQueue;
    } d3d12;

    ComPtr<IDXGIFactory4> dxgiFactory;
    nvrhi::DeviceHandle nvrhi;
    ComPtr<D3D12MA::Allocator> allocator;

    Device();
    ~Device();
};