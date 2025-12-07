#pragma once

#include <d3d9.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <Windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <map>
#include <random>
#include <type_traits>
#include <vector>

#include <D3D12MemAlloc.h>
#include <DDSTextureLoader12.h>
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_upscale.hpp>
#include <ffx_api/ffx_framegeneration.hpp>
#include <ffx_api/dx12/ffx_api_dx12.hpp>
#include <nvsdk_ngx_helpers_dlssd.h>
#include <pix3.h>
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>
#include <IniFile.h>
#include <ankerl/unordered_dense.h>
#include <lz4.h>
#include <nvapi.h>
#include <nvShaderExtnEnums.h>