#pragma once
#include "DebugView.h"

struct RaytracingParams
{
    static inline bool s_enable = true;
    static inline uint32_t s_debugView;

    static inline boost::shared_ptr<Hedgehog::Mirage::CLightData> s_light;
    static inline boost::shared_ptr<Hedgehog::Mirage::CModelData> s_sky;

    static inline float s_diffusePower = 1.0f;
    static inline float s_lightPower = 1.0f;
    static inline float s_emissivePower = 1.0f;
    static inline float s_skyPower = 1.0f;

    static inline uint32_t s_envMode;
    static inline Hedgehog::Math::CVector s_skyColor;
    static inline Hedgehog::Math::CVector s_groundColor;

    static bool update();
};
