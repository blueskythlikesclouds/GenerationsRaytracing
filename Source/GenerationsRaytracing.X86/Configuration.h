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
    static DisplayMode displayMode;
    static bool allowResizeInWindowed;

    static bool load(const std::string& filePath);
};