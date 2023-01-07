#pragma once

struct Params
{
    static inline boost::shared_ptr<hh::mr::CLightData> light;
    static inline boost::shared_ptr<hh::mr::CModelData> sky;

    static bool update();
};
