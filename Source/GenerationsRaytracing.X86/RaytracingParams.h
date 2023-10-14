#pragma once

struct RaytracingParams
{
    static inline bool s_enable = true;

    static inline boost::shared_ptr<Hedgehog::Mirage::CLightData> s_light;
    static inline boost::shared_ptr<Hedgehog::Mirage::CModelData> s_sky;

    static bool update();
};
