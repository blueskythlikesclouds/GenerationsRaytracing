#include "D3D9.h"
#include "PictureData.h"
#include "FillTexture.h"

extern "C" void __declspec(dllexport) Init()
{
#ifdef _DEBUG
    MessageBox(NULL, TEXT("Attach to Process"), TEXT("GenerationsRaytracing"), MB_ICONINFORMATION | MB_OK);

    if (!GetConsoleWindow())
        AllocConsole();

    freopen("CONOUT$", "w", stdout);
#endif

    D3D9::init();
    PictureData::init();
    FillTexture::init();
}
