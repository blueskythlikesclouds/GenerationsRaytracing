#pragma once

struct RaytracingRendering
{
    static inline uint32_t s_frame;

    static void init();
    static void postInit();
};