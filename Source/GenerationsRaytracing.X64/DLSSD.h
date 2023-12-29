#pragma once

#include "DLSS.h"

class DLSSD : public DLSS
{
public:
    DLSSD(const Device& device);

    void init(const InitArgs& args) override;
    void dispatch(const DispatchArgs& args) override;

    UpscalerType getType() override;
};
