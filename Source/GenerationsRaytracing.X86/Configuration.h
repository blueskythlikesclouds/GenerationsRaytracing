#pragma once

#include "QualityMode.h"
#include "UpscalerType.h"

enum class DisplayMode
{
    Borderless = 0,
    BorderlessFullscreen = 1,
    Windowed = 2
};

enum class FurStyle
{
    Frontiers,
    CG
};

class Configuration
{
public:
    static inline bool s_enableRaytracing = true;

    static inline DisplayMode s_displayMode = DisplayMode::BorderlessFullscreen;
    static inline bool s_allowResizeInWindowed = false;

    static inline UpscalerType s_upscaler;
    static inline QualityMode s_qualityMode;

    static inline bool s_gachaLighting;

    static inline bool s_toneMap;

    static inline FurStyle s_furStyle;

    static inline bool s_hdr;

    static inline bool s_enableImgui;

    static void init();
};