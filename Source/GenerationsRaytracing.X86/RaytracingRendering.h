#pragma once

struct RaytracingRendering
{
    static inline Hedgehog::Math::CVector s_worldShift;
    static inline uint32_t s_frame;

    static inline double s_duration;

    static void init();
    static void postInit();
};