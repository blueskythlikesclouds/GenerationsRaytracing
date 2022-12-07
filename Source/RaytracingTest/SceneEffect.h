#pragma once

struct SceneEffect
{
    struct Default
    {
        float skyIntensityScale = 1.0f;
    } defaults;

    struct HDR
    {
        float middleGray = 0.37f;
        float lumMin = 0.15f;
        float lumMax = 1.74f;
    } hdr;

    struct LightScattering
    {
        bool enable = false;
        Eigen::Vector4f color = { 0.1f, 0.21f, 0.3f, 0.0f }; // ms_Color
        float depthScale = 9.1f;                             // ms_FarNearScale.z
        float inScatteringScale = 50.0f;                     // ms_FarNearScale.w
        float rayleigh = 0.1f;                               // ms_Ray_Mie_Ray2_Mie2.x
        float mie = 0.01f;                                   // ms_Ray_Mie_Ray2_Mie2.y
        float g = 0.7f;                                      // ms_G
        float zNear = 60.0f;                                 // ms_FarNearScale.y
        float zFar = 700.0f;                                 // ms_FarNearScale.x

        // Values passed to GPU
        struct GPU
        {
            Eigen::Vector4f rayMieRay2Mie2 = Eigen::Vector4f::Zero();
            Eigen::Vector4f gAndFogDensity = Eigen::Vector4f::Zero();
            Eigen::Vector4f farNearScale = Eigen::Vector4f::Zero();
        } gpu;

        void computeGpuValues();

    } lightScattering;

    void load(const tinyxml2::XMLDocument& doc);
};
