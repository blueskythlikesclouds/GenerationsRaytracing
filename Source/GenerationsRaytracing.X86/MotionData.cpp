#include "MotionData.h"

HOOK(Hedgehog::Motion::CMaterialMotion*, __fastcall, MaterialMotionCtor, 0x756FE0, Hedgehog::Motion::CMaterialMotion* This)
{
    memset(This, 0, sizeof(Hedgehog::Motion::CMaterialMotion));
    return originalMaterialMotionCtor(This);
}

void MotionData::init()
{
    INSTALL_HOOK(MaterialMotionCtor);
}
