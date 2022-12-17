#pragma once

#include <BlueBlur.h>

#include <detours.h>
#include <Windows.h>

#include <wrl/client.h>

#include <dxgi1_2.h>
#include <d3d9.h>

#include <cstdio>
#include <mutex>

#include <Helpers.h>
#include <INIReader.h>

#if _DEBUG
#define PRINT(x, ...) printf(x, __VA_ARGS__);
#else
#define PRINT(x, ...)
#endif

#define FUNCTION_STUB(returnType, name, ...) \
    returnType name(__VA_ARGS__) \
    { \
        PRINT("GenerationsRaytracing: %s() stubbed\n", #name); \
        return (returnType)E_FAIL; \
    }

using Microsoft::WRL::ComPtr;