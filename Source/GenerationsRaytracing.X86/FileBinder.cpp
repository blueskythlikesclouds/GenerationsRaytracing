#include "FileBinder.h"

#include "Configuration.h"

void FileBinder::init(ModInfo_t* modInfo)
{
    modInfo->API->BindFile(modInfo->CurrentMod, "+shader_debug.arl", "disk/bb3/+shader_r.arl", 0);
    modInfo->API->BindFile(modInfo->CurrentMod, "+shader_s.arl", "disk/bb3/+shader_r.arl", 0);

    if (Configuration::s_enableRaytracing)
    {
        const char* sonicFurArchive = Configuration::s_furStyle == FurStyle::CG ?
            "disk/bb3/SonicFurCG.ar.00" : "disk/bb3/SonicFurFrontiers.ar.00";

        const char* sonicFurArchiveList = Configuration::s_furStyle == FurStyle::CG ?
            "disk/bb3/SonicFurCG.arl" : "disk/bb3/SonicFurFrontiers.arl";

        const char* sonicClassicFurArchive = Configuration::s_furStyle == FurStyle::CG ?
            "disk/bb3/SonicClassicFurCG.ar.00" : "disk/bb3/SonicClassicFurFrontiers.ar.00";

        const char* sonicClassicFurArchiveList = Configuration::s_furStyle == FurStyle::CG ?
            "disk/bb3/SonicClassicFurCG.arl" : "disk/bb3/SonicClassicFurFrontiers.arl";

        modInfo->API->BindFile(modInfo->CurrentMod, "SonicFur.ar.00", sonicFurArchive, 0);
        modInfo->API->BindFile(modInfo->CurrentMod, "SonicFur.arl", sonicFurArchiveList, 0);

        modInfo->API->BindFile(modInfo->CurrentMod, "SonicClassicFur.ar.00", sonicClassicFurArchive, 0);
        modInfo->API->BindFile(modInfo->CurrentMod, "SonicClassicFur.arl", sonicClassicFurArchiveList, 0);
    }
}
