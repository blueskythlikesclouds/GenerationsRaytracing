#include "Configuration.h"

void Configuration::init()
{
    const INIReader reader("GenerationsRaytracing.ini");

    if (reader.ParseError() != 0)
    {
        MessageBox(nullptr, TEXT("Unable to locate GenerationsRaytracing.ini"), TEXT("GenerationsRaytracing"), MB_ICONERROR);
    }
    else
    {
        s_displayMode = static_cast<DisplayMode>(reader.GetInteger("Mod", "DisplayMode", static_cast<uint32_t>(DisplayMode::BorderlessFullscreen)));
        s_allowResizeInWindowed = reader.GetBoolean("Mod", "AllowResizeInWindowed", false);
    }
}
