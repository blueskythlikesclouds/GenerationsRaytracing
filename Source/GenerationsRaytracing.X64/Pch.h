#pragma once

#include <d3d9.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
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
#include <ffx_fsr2.h>
#include <INIReader.h>
#include <nvsdk_ngx_helpers_dlssd.h>
#include <pix3.h>
#include <xxhash.h>
#include <ankerl/unordered_dense.h>
#include <dx12/ffx_fsr2_dx12.h>
