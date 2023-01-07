#pragma once

struct Stage
{
    static inline boost::shared_ptr<hh::mr::CLightData> light;
    static inline boost::shared_ptr<hh::mr::CModelData> sky;

    static bool setSceneEffect();

    static void setStage(size_t index);
    static void setStage(const std::string& name);
};
