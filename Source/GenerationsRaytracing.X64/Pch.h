#pragma once

#include <Windows.h>
#include <detours.h>

#include <wrl/client.h>

#include <d3d9.h>
#include <d3dx12.h>
#include <dxgi1_4.h>

#include <cstdio>
#include <unordered_map>
#include <unordered_set>

#include <nvrhi/common/misc.h>
#include <nvrhi/d3d12.h>
#include <nvrhi/utils.h>
#include <nvrhi/validation.h>

#include <DDSTextureLoader12.h>
#include <D3D12MemAlloc.h>

#ifndef _DEBUG
#define inline __forceinline
#include <xxHash.h>
#undef inline
#else
#include <xxHash.h>
#endif

#include <Helpers.h>

#include <nvsdk_ngx_helpers.h>
#include <ffx_fsr2.h>
#include <dx12/ffx_fsr2_dx12.h>

#include <INIReader.h>

#include <unordered_dense.h>

struct XXHash
{
    using is_avalanching = void;

    uint64_t operator()(XXH64_hash_t const& x) const noexcept
    {
        return x;
    }
};

template<typename T>
using XXHashMap = ankerl::unordered_dense::map<XXH64_hash_t, T, XXHash>;

using Microsoft::WRL::ComPtr;