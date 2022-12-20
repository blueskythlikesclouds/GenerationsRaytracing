#include "MemoryAllocator.h"
#include "CriticalSection.h"

static ComPtr<D3D12MA::Allocator> allocator;
static std::unordered_map<void*, ComPtr<D3D12MA::Allocation>> allocations;
static bool callOriginal;
static CriticalSection criticalSection;

VTABLE_HOOK(HRESULT, STDMETHODCALLTYPE, ID3D12Device, CreateCommittedResource,
    const D3D12_HEAP_PROPERTIES* pHeapProperties,
    D3D12_HEAP_FLAGS HeapFlags,
    const D3D12_RESOURCE_DESC* pDesc,
    D3D12_RESOURCE_STATES InitialResourceState,
    const D3D12_CLEAR_VALUE* pOptimizedClearValue,
    REFIID riidResource,
    void** ppvResource)
{
    std::lock_guard lock(criticalSection);

    if (callOriginal)
    {
        return originalID3D12DeviceCreateCommittedResource(
            This,
            pHeapProperties,
            HeapFlags,
            pDesc,
            InitialResourceState,
            pOptimizedClearValue,
            riidResource,
            ppvResource);
    }

    D3D12MA::ALLOCATION_DESC desc{};
    desc.HeapType = pHeapProperties->Type;

    ComPtr<D3D12MA::Allocation> allocation;

    callOriginal = true;

    const HRESULT result = allocator->CreateResource(
        &desc,
        pDesc,
        InitialResourceState,
        pOptimizedClearValue,
        allocation.GetAddressOf(),
        riidResource,
        ppvResource);

    callOriginal = false;

    if (FAILED(result))
        return result;

    allocations[*ppvResource] = std::move(allocation);
    return result;
}

void MemoryAllocator::init(ID3D12Device* device, IDXGIAdapter* adapter)
{
    D3D12MA::ALLOCATOR_DESC desc{};
    desc.pDevice = device;
    desc.pAdapter = adapter;

    D3D12MA::CreateAllocator(&desc, allocator.GetAddressOf());

    INSTALL_VTABLE_HOOK(ID3D12Device, device, CreateCommittedResource, 27);
}
