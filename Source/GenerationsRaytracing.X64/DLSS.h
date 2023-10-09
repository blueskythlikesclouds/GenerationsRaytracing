#pragma once
#include "Upscaler.h"

class DLSS : public Upscaler
{
protected:
    ComPtr<ID3D12Device> m_device;
    NVSDK_NGX_Parameter* m_parameters = nullptr;
    NVSDK_NGX_Handle* m_feature = nullptr;

public:
    DLSS(const Device& device);
    ~DLSS() override;
    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;
};
