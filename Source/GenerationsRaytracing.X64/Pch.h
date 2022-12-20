#pragma once

#include <Windows.h>
#include <detours.h>

#include <wrl/client.h>

#include <d3d9.h>
#include <dxgi1_4.h>

#include <cstdio>
#include <unordered_map>

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

using Microsoft::WRL::ComPtr;