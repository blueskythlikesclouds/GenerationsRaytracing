#pragma once

struct RaytracingParams
{
    static inline bool s_enable = true;

    static inline boost::shared_ptr<Hedgehog::Mirage::CLightData> s_light;
    static inline boost::shared_ptr<Hedgehog::Mirage::CModelData> s_sky;

    static inline float s_diffusePower = 1.0f;
    static inline float s_lightPower = 1.0f;
    static inline float s_emissivePower = 1.0f;

    static bool update();
};
