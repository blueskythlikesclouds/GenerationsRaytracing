﻿#pragma once

#include "DebugView.h"
#include "QualityMode.h"
#include "UpscalerType.h"

enum ToneMapMode
{
    TONE_MAP_MODE_UNSPECIFIED,
    TONE_MAP_MODE_ENABLE,
    TONE_MAP_MODE_DISABLE
};

struct RaytracingParams
{
    static inline bool s_enable = true;
    static inline uint32_t s_debugView;

    static inline boost::shared_ptr<Hedgehog::Mirage::CLightData> s_light;
    static inline boost::shared_ptr<Hedgehog::Mirage::CModelData> s_sky;

    static inline float s_diffusePower = 1.0f;
    static inline float s_lightPower = 1.0f;
    static inline float s_emissivePower = 1.0f;
    static inline float s_skyPower = 1.0f;

    static inline uint32_t s_envMode;
    static inline Hedgehog::Math::CVector s_skyColor;
    static inline Hedgehog::Math::CVector s_groundColor;
    static inline bool s_skyInRoughReflection = true;

    static inline UpscalerType s_upscaler;
    static inline QualityMode s_qualityMode;

    static inline uint32_t s_toneMapMode;

    // Debug
    static inline bool s_enableNoAoModels = true;
    static inline bool s_computeSmoothNormals = true;
    static inline bool s_prevComputeSmoothNormals = true;

    static bool update();

    static void imguiWindow();

    static void init();
};
