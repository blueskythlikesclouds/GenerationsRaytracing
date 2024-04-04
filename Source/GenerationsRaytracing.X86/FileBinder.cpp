#include "FileBinder.h"

#include "Configuration.h"

void FileBinder::init(ModInfo_t* modInfo)
{
    const char* sonicFurArchive = Configuration::s_furStyle == FurStyle::CG ? 
        "disk/bb3/SonicFurCG.arl" : "disk/bb3/SonicFurFrontiers.arl";

    const char* sonicClassicFurArchive = Configuration::s_furStyle == FurStyle::CG ?
        "disk/bb3/SonicClassicFurCG.arl" : "disk/bb3/SonicClassicFurFrontiers.arl";

    modInfo->API->BindFile(modInfo->CurrentMod, "+Title.arl", sonicFurArchive, 0);
    modInfo->API->BindFile(modInfo->CurrentMod, "+Sonic.arl", sonicFurArchive, 0);
    modInfo->API->BindFile(modInfo->CurrentMod, "+evSonic.arl", sonicFurArchive, 0);

    modInfo->API->BindFile(modInfo->CurrentMod, "+Title.arl", sonicClassicFurArchive, 0);
    modInfo->API->BindFile(modInfo->CurrentMod, "+SonicClassic.arl", sonicClassicFurArchive, 0);
    modInfo->API->BindFile(modInfo->CurrentMod, "+evSonicClassic.arl", sonicClassicFurArchive, 0);
}
