#pragma once

struct RaytracingParams
{
    static inline bool enable = true;

    static inline boost::shared_ptr<hh::mr::CLightData> light;
    static inline boost::shared_ptr<hh::mr::CModelData> sky;

    static bool update();
};
