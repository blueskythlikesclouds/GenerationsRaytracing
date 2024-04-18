#include "NGX.h"
#include "Device.h"

static NGX* s_ngx;
static ankerl::unordered_dense::map<IUnknown*, ComPtr<D3D12MA::Allocation>> s_allocations;

static void resourceAllocCallback(D3D12_RESOURCE_DESC* desc, int state, CD3DX12_HEAP_PROPERTIES* heap, ID3D12Resource** resource)
{
    D3D12MA::ALLOCATION_DESC allocDesc{};
    allocDesc.HeapType = heap->Type;

    ComPtr<D3D12MA::Allocation> allocation;

    HRESULT hr = s_ngx->getDevice().getAllocator()->CreateResource(
        &allocDesc,
        desc,
        static_cast<D3D12_RESOURCE_STATES>(state),
        nullptr,
        allocation.GetAddressOf(),
        __uuidof(ID3D12Resource),
        nullptr);

    assert(SUCCEEDED(hr) && allocation != nullptr);

#ifdef _DEBUG
    wchar_t name[0x100];
    swprintf_s(name, L"NGX Resource %p", allocation->GetResource());
    allocation->GetResource()->SetName(name);
#endif

    *resource = allocation->GetResource();
    s_allocations.emplace(allocation->GetResource(), std::move(allocation));
}

static void resourceReleaseCallback(IUnknown* resource)
{
    s_allocations.erase(resource);
}

NGX::NGX(const Device& device) : m_device(device)
{
    s_ngx = this;

    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_Init(0x205452736E654720, L".", device.getUnderlyingDevice())))
        return;

    m_valid = true;

    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&m_parameters)))
        return;

    m_parameters->Set(NVSDK_NGX_Parameter_ResourceAllocCallback, resourceAllocCallback);
    m_parameters->Set(NVSDK_NGX_Parameter_ResourceReleaseCallback, resourceReleaseCallback);

    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    m_parameters->Set(NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);

    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_DLAA, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Quality, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Balanced, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_Performance, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraPerformance, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
    m_parameters->Set(NVSDK_NGX_Parameter_RayReconstruction_Hint_Render_Preset_UltraQuality, NVSDK_NGX_RayReconstruction_Hint_Render_Preset_A);
}

NGX::~NGX()
{
    if (m_parameters != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(m_parameters));

    if (m_valid)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown1(m_device.getUnderlyingDevice()));

    s_ngx = nullptr;
}

const Device& NGX::getDevice() const
{
    return m_device;
}

NVSDK_NGX_Parameter* NGX::getParameters() const
{
    return m_parameters;
}

bool NGX::valid() const
{
    return m_valid && m_parameters != nullptr;
}
