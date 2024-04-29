#include "Configuration.h"

void Configuration::init()
{
    IniFile iniFile;

    if (!iniFile.read("GenerationsRaytracing.ini"))
    {
        MessageBox(nullptr, TEXT("Unable to read GenerationsRaytracing.ini"), TEXT("GenerationsRaytracing"), MB_ICONERROR);
    }
    else
    {
        s_displayMode = static_cast<DisplayMode>(iniFile.get<uint32_t>("Mod", "DisplayMode", static_cast<uint32_t>(DisplayMode::BorderlessFullscreen)));
        s_allowResizeInWindowed = iniFile.getBool("Mod", "AllowResizeInWindowed", false);
        s_upscaler = iniFile.get<uint32_t>("Mod", "Upscaler", 2);
        s_qualityMode = iniFile.get<uint32_t>("Mod", "QualityMode", 2);
        s_gachaLighting = iniFile.getBool("Mod", "GachaLighting", false);
        s_toneMap = iniFile.getBool("Mod", "ToneMap", true);
        s_furStyle = static_cast<FurStyle>(iniFile.get<uint32_t>("Mod", "FurStyle", static_cast<uint32_t>(FurStyle::Frontiers)));
        s_hdr = iniFile.getBool("Mod", "HDR", false);
    }
}
