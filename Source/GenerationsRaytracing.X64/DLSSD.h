#pragma once

#include "DLSS.h"

class DLSSD : public DLSS
{
public:
    DLSSD(const NGX& ngx);

    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;

    UpscalerType getType() override;

    static bool valid(const NGX& ngx);
};
