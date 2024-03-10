#pragma once

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
    static inline DisplayMode s_displayMode = DisplayMode::BorderlessFullscreen;
    static inline bool s_allowResizeInWindowed = false;

    static inline uint32_t s_upscaler;
    static inline uint32_t s_qualityMode;

    static inline bool s_gachaLighting;

    static inline bool s_toneMap;

    static inline FurStyle s_furStyle;

    static inline bool s_enableImgui;

    static void init();
};