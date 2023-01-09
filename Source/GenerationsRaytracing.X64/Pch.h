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

#include <hash_table8.hpp>

using Microsoft::WRL::ComPtr;