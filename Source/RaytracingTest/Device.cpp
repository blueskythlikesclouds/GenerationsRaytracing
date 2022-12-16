#include "Device.h"

static ID3D12CommandQueue* createCommandQueue(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
    
    commandQueueDesc.Type = type;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = 0;

    ID3D12CommandQueue* commandQueue = nullptr;
    device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));

    assert(commandQueue);
    return commandQueue;
}

static class MessageCallback : public nvrhi::IMessageCallback
{
public:
    void message(nvrhi::MessageSeverity severity, const char* messageText) override
    {
        constexpr const char* SEVERITY_TEXT[] =
        {
            "Info",
            "Warning",
            "Error",
            "Fatal"
};

        printf("%s: %s\n", SEVERITY_TEXT[(int)severity], messageText);
        assert(severity != nvrhi::MessageSeverity::Error && severity != nvrhi::MessageSeverity::Fatal);
    }
} messageCallback;

Device::Device()
{
#if _DEBUG
    ComPtr<ID3D12Debug> debugInterface;
    D3D12GetDebugInterface(IID_PPV_ARGS(debugInterface.GetAddressOf()));
    assert(debugInterface);

    debugInterface->EnableDebugLayer();
#endif

    CreateDXGIFactory2(
#if _DEBUG
        DXGI_CREATE_FACTORY_DEBUG,
#else
        0,
#endif
        IID_PPV_ARGS(dxgiFactory.GetAddressOf()));

    assert(dxgiFactory);

    ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiFactory->EnumAdapters(0, dxgiAdapter.GetAddressOf());
    assert(dxgiAdapter);

    D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(d3d12.device.GetAddressOf()));
    assert(d3d12.device);

    d3d12.graphicsCommandQueue.Attach(createCommandQueue(d3d12.device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT));
    d3d12.computeCommandQueue.Attach(createCommandQueue(d3d12.device.Get(), D3D12_COMMAND_LIST_TYPE_COMPUTE));
    d3d12.copyCommandQueue.Attach(createCommandQueue(d3d12.device.Get(), D3D12_COMMAND_LIST_TYPE_COPY));

    nvrhi::d3d12::DeviceDesc deviceDesc;
    deviceDesc.errorCB = &messageCallback;
    deviceDesc.pDevice = d3d12.device.Get();
    deviceDesc.pGraphicsCommandQueue = d3d12.graphicsCommandQueue.Get();
    deviceDesc.pComputeCommandQueue = d3d12.computeCommandQueue.Get();
    deviceDesc.pCopyCommandQueue = d3d12.copyCommandQueue.Get();

    nvrhi = nvrhi::d3d12::createDevice(deviceDesc);
    assert(nvrhi);

#if _DEBUG
    nvrhi = nvrhi::validation::createValidationLayer(nvrhi);
    assert(nvrhi);
#endif
}

Device::~Device()
{
    nvrhi->waitForIdle();
}