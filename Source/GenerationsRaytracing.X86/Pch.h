#pragma once

#include <d3d9.h>
#include <Windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_set>

#include <BlueBlur.h>
#include <detours.h>
#include <Helpers.h>
#include <INIReader.h>
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

#define FUNCTION_STUB(RETURN_TYPE, RETURN_VALUE, FUNCTION_NAME, ...) \
    RETURN_TYPE FUNCTION_NAME(__VA_ARGS__) { assert(!#FUNCTION_NAME); return RETURN_VALUE; }