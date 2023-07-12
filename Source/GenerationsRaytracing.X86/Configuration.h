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

    static void init();
};