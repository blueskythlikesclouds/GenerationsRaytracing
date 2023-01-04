#pragma once

#define NRI_THROW_IF_FAILED(x) \
    do \
    { \
        const nri::Result result = x; \
        if (result != nri::Result::SUCCESS) \
        { \
            printf(#x " failed with error code %x\n", result); \
            assert(0 && #x); \
        } \
    } while (0)

class MemoryAllocator;

struct NriInterface
    : nri::CoreInterface
    , nri::HelperInterface
    , nri::WrapperD3D12Interface
{
};

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

    nri::Device* nriDevice = nullptr;
    NriInterface nriInterface{};

    Device();
    ~Device();
};