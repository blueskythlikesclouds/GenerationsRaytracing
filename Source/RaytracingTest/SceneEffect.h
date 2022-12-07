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

    struct EyeLight
    {
        Eigen::Vector4f diffuse = { 0.1f, 0.1f, 0.1f, 1.0f };  // EyeLightDiffuse
        Eigen::Vector4f specular = { 0.1f, 0.1f, 0.1f, 1.0f }; // EyeLightSpecular
        uint32_t mode = 1;                                     // EyeLightMode
        float farRangeStart = 0.0f;                            // EyeLightRange.z
        float farRangeEnd = 57.0f;                             // EyeLightRange.w
        float innerDiameter = 90.0f;                           // EyeLightInnterDiameter
        float outerDiameter = 90.0f;                           // EyeLightOuterDiameter
        float falloff = 2.0f;                                  // EyeLightAttribute.w

        struct GPU
        {
            Eigen::Vector4f range = Eigen::Vector4f::Zero();
            Eigen::Vector4f attribute = Eigen::Vector4f::Zero();
        } gpu;

        void computeGpuValues();
    } eyeLight;

    void load(const tinyxml2::XMLDocument& doc);
};
