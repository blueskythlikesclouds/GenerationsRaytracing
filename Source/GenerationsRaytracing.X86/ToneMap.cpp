#include "ToneMap.h"
#include "Configuration.h"
#include "RaytracingParams.h"

static FUNCTION_PTR(int32_t, __cdecl, getShaderIndex, 0x64E560, void*, void*);
static FUNCTION_PTR(void, __cdecl, screenDefaultExec, 0x651520, void*, void*, int32_t);

static void screenRenderParamCallback(void* A1, Hedgehog::FxRenderFramework::SScreenRenderParam* screenRenderParam)
{
    int32_t shaderIndex = 0;

    if ((Configuration::s_toneMap && 
        RaytracingParams::s_toneMapMode != TONE_MAP_MODE_DISABLE) || RaytracingParams::s_toneMapMode == TONE_MAP_MODE_ENABLE)
    {
        shaderIndex = screenRenderParam->ShaderIndex;
    }

    screenDefaultExec(A1, screenRenderParam, shaderIndex);
}

Hedgehog::FxRenderFramework::SScreenRenderParam s_screenRenderParam =
{
    "tone map",
    screenRenderParamCallback,
    0,
    {}
};

static FUNCTION_PTR(void, __thiscall, makeShaderPair, 0x654480,
    void* This, int32_t index, Hedgehog::Database::CDatabase* database, const char* vertexShaderName, const char* pixelShaderName);

HOOK(void, __fastcall, MakeShaderList, 0x654590, void* This, void* _, Hedgehog::Database::CDatabase* database)
{
    memset(static_cast<uint8_t*>(This) + 
        s_screenRenderParam.ShaderIndex * sizeof(Hedgehog::Mirage::SShaderPair), 0, sizeof(Hedgehog::Mirage::SShaderPair));

    makeShaderPair(This, s_screenRenderParam.ShaderIndex, database, "FxFilterT", "FxToneMap");

    originalMakeShaderList(This, _, database);
}

static uint32_t* s_shaderListByteSize = reinterpret_cast<uint32_t*>(0x6514E3);

void ToneMap::postInit()
{
    s_screenRenderParam.ShaderIndex = *s_shaderListByteSize / sizeof(Hedgehog::Mirage::SShaderPair);
    WRITE_MEMORY(s_shaderListByteSize, uint32_t, *s_shaderListByteSize + sizeof(Hedgehog::Mirage::SShaderPair));
    INSTALL_HOOK(MakeShaderList);
    WRITE_MEMORY(0x13DD70C, void*, &s_screenRenderParam);
}