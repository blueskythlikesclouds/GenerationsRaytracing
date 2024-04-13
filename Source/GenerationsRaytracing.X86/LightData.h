#pragma once

struct Light
{
    std::string name;
    float position[3];
    float color[3];
    float colorIntensity;
    float inRange;
    float outRange;
};

struct LightData
{
    static inline std::vector<Light> s_lights;
    static inline bool s_dirty = false;

    static void update(XXH32_hash_t hash);
    static void init(ModInfo_t* modInfo);

    static void renderImgui();
};