#include "NGX.h"
#include "Device.h"

NGX::NGX(const Device& device)
{
    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_Init(0x205452736E654720, L".", device.getUnderlyingDevice())))
        return;

    m_device = device.getUnderlyingDevice();

    if (NVSDK_NGX_FAILED(NVSDK_NGX_D3D12_GetCapabilityParameters(&m_parameters)))
        return;

    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
    NVSDK_NGX_Parameter_SetUI(m_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality, NVSDK_NGX_DLSS_Hint_Render_Preset_C);
}

NGX::~NGX()
{
    if (m_parameters != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_DestroyParameters(m_parameters));

    if (m_device != nullptr)
        THROW_IF_FAILED(NVSDK_NGX_D3D12_Shutdown1(m_device.Get()));
}

NVSDK_NGX_Parameter* NGX::getParameters() const
{
    return m_parameters;
}

bool NGX::valid() const
{
    return m_parameters != nullptr;
}
