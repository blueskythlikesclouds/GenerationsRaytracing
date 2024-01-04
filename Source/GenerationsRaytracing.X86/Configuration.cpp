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
        s_upscaler = static_cast<uint32_t>(reader.GetInteger("Mod", "Upscaler", 1));
        s_qualityMode = static_cast<uint32_t>(reader.GetInteger("Mod", "QualityMode", 2));
        s_gachaLighting = reader.GetBoolean("Mod", "GachaLighting", false);
        s_toneMap = reader.GetBoolean("Mod", "ToneMap", false);
    }
}
