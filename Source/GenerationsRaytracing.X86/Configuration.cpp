#include "Configuration.h"

DisplayMode Configuration::displayMode = DisplayMode::BorderlessFullscreen;
bool Configuration::allowResizeInWindowed = false;

bool Configuration::load(const std::string& filePath)
{
    const INIReader reader(filePath);
    if (reader.ParseError() != 0)
        return false;

    displayMode = (DisplayMode)reader.GetInteger("Mod", "DisplayMode", (uint32_t)DisplayMode::BorderlessFullscreen);
    allowResizeInWindowed = reader.GetBoolean("Mod", "AllowResizeInWindowed", false);

    return true;
}
