#pragma once

struct Light
{
    std::string name;
    Hedgehog::Math::CVector position{};
    float color[3]{};
    float colorIntensity{};
    float inRange{};
    float outRange{};
    bool castShadow{};
    bool enableBackfaceCulling{};
    float shadowRange{};
};

struct LightData
{
    static inline std::vector<Light> s_lights;

    static void update(XXH32_hash_t hash);
    static void init(ModInfo_t* modInfo);

    static void renderImgui();
};