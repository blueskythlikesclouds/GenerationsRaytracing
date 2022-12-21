#pragma once

#define _USE_MATH_DEFINES
#include <corecrt_math.h>

#include <Windows.h>
#include <Windowsx.h>

#include <wrl/client.h>

#include <dxgi1_4.h>
#include <DDSTextureLoader/DDSTextureLoader12.h>

#include <fdi.h>

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <Eigen/Eigen>

#include <nvrhi/d3d12.h>
#include <nvrhi/utils.h>
#include <nvrhi/validation.h>

#pragma warning(push, 0)

#include <hedgelib/archives/hl_archive.h>
#include <hedgelib/archives/hl_hh_archive.h>
#include <hedgelib/hh/hl_hh_light.h>
#include <hedgelib/io/hl_mem_stream.h>
#include <hedgelib/io/hl_path.h>
#include <hedgelib/materials/hl_hh_material.h>
#include <hedgelib/models/hl_hh_model.h>
#include <hedgelib/terrain/hl_hh_terrain.h>

#pragma warning(pop)

#include <tinyxml2/tinyxml2.h>

#include <nvsdk_ngx_helpers.h>

#include <D3D12MemAlloc.h>

using Microsoft::WRL::ComPtr;