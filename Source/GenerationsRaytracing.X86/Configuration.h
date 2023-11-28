#pragma once

enum class DisplayMode
{
    Borderless = 0,
    BorderlessFullscreen = 1,
    Windowed = 2
};

class Configuration
{
public:
    static inline DisplayMode s_displayMode = DisplayMode::BorderlessFullscreen;
    static inline bool s_allowResizeInWindowed = false;

    static inline uint32_t s_upscaler;
    static inline uint32_t s_qualityMode;

    static inline bool s_gachaLighting;

    static void init();
};