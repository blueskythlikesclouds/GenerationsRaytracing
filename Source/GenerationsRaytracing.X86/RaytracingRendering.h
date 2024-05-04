#pragma once

struct RaytracingRendering
{
    static inline Hedgehog::Math::CVector s_worldShift;
    static inline uint64_t s_frame;

    static void init();
    static void postInit();
};