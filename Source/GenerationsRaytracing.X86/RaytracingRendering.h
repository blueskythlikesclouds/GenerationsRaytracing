#pragma once

struct RaytracingRendering
{
    static inline uint32_t s_frame;

    static void preInit();
    static void init();
    static void postInit();
};