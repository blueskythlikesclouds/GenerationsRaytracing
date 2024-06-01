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
        s_enableRaytracing = iniFile.getBool("Mod", "EnableRaytracing", true);
        s_displayMode = static_cast<DisplayMode>(iniFile.get<uint32_t>("Mod", "DisplayMode", static_cast<uint32_t>(DisplayMode::BorderlessFullscreen)));
        s_allowResizeInWindowed = iniFile.getBool("Mod", "AllowResizeInWindowed", false);
        s_upscaler = static_cast<UpscalerType>(iniFile.get<uint32_t>("Mod", "Upscaler", static_cast<uint32_t>(UpscalerType::FSR3)));
        s_qualityMode = static_cast<QualityMode>(iniFile.get<uint32_t>("Mod", "QualityMode", static_cast<uint32_t>(QualityMode::Auto)));
        s_gachaLighting = iniFile.getBool("Mod", "GachaLighting", false);
        s_toneMap = iniFile.getBool("Mod", "ToneMap", true);
        s_furStyle = static_cast<FurStyle>(iniFile.get<uint32_t>("Mod", "FurStyle", static_cast<uint32_t>(FurStyle::Frontiers)));
        s_hdr = iniFile.getBool("Mod", "HDR", false);
    }
}
