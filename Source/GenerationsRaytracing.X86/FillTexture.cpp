#include "FillTexture.h"

class Texture;

typedef void(__stdcall* LPD3DXFILL2D)(
    Hedgehog::Math::CVector4* out,
    const Hedgehog::Math::CVector2* texCoord,
    const Hedgehog::Math::CVector2* texelSize,
    LPVOID data);

HOOK(HRESULT, __stdcall, FillTexture, 0xA55270, Texture* texture, LPD3DXFILL2D function, LPVOID data)
{
    return E_NOTIMPL;
}

void FillTexture::init()
{
    INSTALL_HOOK(FillTexture);
}
