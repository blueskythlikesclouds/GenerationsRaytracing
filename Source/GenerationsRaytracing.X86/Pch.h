#pragma once

#include <d3d9.h>
#include <Windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <vector>
#include <thread>
#include <unordered_set>
#include <random>

#include <BlueBlur.h>
#include <detours.h>
#include <Helpers.h>
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>
#include <IniFile.h>
#include <HE1ML/ModLoader.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <implot.h>
#include <im3d.h>

#include <json/json.hpp>
using nlohmann::json;

#define FUNCTION_STUB(RETURN_TYPE, RETURN_VALUE, FUNCTION_NAME, ...) \
    RETURN_TYPE FUNCTION_NAME(__VA_ARGS__) { assert(!#FUNCTION_NAME); return RETURN_VALUE; }

struct xxHash
{
    std::size_t operator()(XXH32_hash_t value) const
    {
        return value;
    }
};

template<typename T>
using xxHashMap = std::unordered_map<XXH32_hash_t, T, xxHash>;