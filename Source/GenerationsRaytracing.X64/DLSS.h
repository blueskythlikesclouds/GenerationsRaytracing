#pragma once

#include "NGX.h"
#include "Upscaler.h"

class DLSS : public Upscaler
{
protected:
    const NGX& m_ngx;
    NVSDK_NGX_Handle* m_feature = nullptr;

    static NVSDK_NGX_PerfQuality_Value getPerfQualityValue(QualityMode qualityMode);

public:
    DLSS(const NGX& ngx);
    ~DLSS() override;

    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;

    UpscalerType getType() override;

    static bool valid(const NGX& ngx);
};
