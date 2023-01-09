#pragma once

#include <BlueBlur.h>

#include <detours.h>
#include <Windows.h>

#include <wrl/client.h>

#include <dxgi1_2.h>
#include <d3d9.h>

#include <cstdio>
#include <mutex>
#include <unordered_set>

#include <Helpers.h>
#include <INIReader.h>

#include <oneapi/tbb.h>

#include <hash_table8.hpp>

#pragma warning(push, 0)

#include <hedgelib/models/hl_hh_model.h>

#pragma warning(pop)

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