#pragma once
#include "Upscaler.h"

#ifdef _DEBUG

#define THROW_IF_FAILED(x) \
    do \
    { \
        const NVSDK_NGX_Result result = x; \
        if (NVSDK_NGX_FAILED(result)) \
        { \
            OutputDebugStringW(GetNGXResultAsString(result)); \
            assert(!#x); \
        } \
    } while (0)

#else
#define THROW_IF_FAILED(x) x
#endif

class DLSS : public Upscaler
{
protected:
    ComPtr<ID3D12Device> m_device;
    NVSDK_NGX_Parameter* m_parameters = nullptr;
    NVSDK_NGX_Handle* m_feature = nullptr;

    static NVSDK_NGX_PerfQuality_Value getPerfQualityValue(QualityMode qualityMode);

public:
    DLSS(const Device& device);
    ~DLSS() override;

    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;

    UpscalerType getType() override;

    bool valid() const;
};
