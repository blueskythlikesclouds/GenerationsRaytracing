#pragma once

#include <d3dx12.h>
#include <d3d9.h>
#include <dxgi1_4.h>
#include <Windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <type_traits>
#include <vector>
#include <map>
#include <random>

#include <D3D12MemAlloc.h>
#include <DDSTextureLoader12.h>
#include <ankerl/unordered_dense.h>
#include <xxhash.h>