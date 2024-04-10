#include "GroundSmokeParticle.h"

HOOK(void, __fastcall, SmokePrimitiveRender, 0x11A2750, void* This, void* _, 
    Hedgehog::Mirage::CRenderInfo* renderInfo, void* A3, void* A4)
{
    renderInfo->m_pRenderingDevice->m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    renderInfo->m_pRenderingDevice->m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    renderInfo->m_pRenderingDevice->m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

    return originalSmokePrimitiveRender(This, _, renderInfo, A3, A4);
}

void GroundSmokeParticle::init()
{
    INSTALL_HOOK(SmokePrimitiveRender);
}