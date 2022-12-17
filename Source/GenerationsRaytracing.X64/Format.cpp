#include "Format.h"

nvrhi::Format Format::convert(unsigned int format)
{
    switch (format)
    {
    case D3DFMT_A16B16G16R16: return nvrhi::Format::RGBA16_UNORM;
    case D3DFMT_A16B16G16R16F: return nvrhi::Format::RGBA16_FLOAT;
    case D3DFMT_A1R5G5B5: return nvrhi::Format::B5G5R5A1_UNORM;
    case D3DFMT_A2B10G10R10: return nvrhi::Format::R10G10B10A2_UNORM;
    case D3DFMT_A32B32G32R32F: return nvrhi::Format::RGBA32_FLOAT;
    case D3DFMT_A4R4G4B4: return nvrhi::Format::BGRA4_UNORM;
    case D3DFMT_A8: return nvrhi::Format::R8_UNORM;
    case D3DFMT_A8B8G8R8: return nvrhi::Format::RGBA8_UNORM;
    case D3DFMT_A8R8G8B8: return nvrhi::Format::BGRA8_UNORM;
    case D3DFMT_ATI1: return nvrhi::Format::BC4_UNORM;
    case D3DFMT_ATI2: return nvrhi::Format::BC5_UNORM;
    case D3DFMT_D15S1: return nvrhi::Format::D24S8;
    case D3DFMT_D16: return nvrhi::Format::D16;
    case D3DFMT_D16_LOCKABLE: return nvrhi::Format::D16;
    case D3DFMT_D24FS8: return nvrhi::Format::D24S8;
    case D3DFMT_D24S8: return nvrhi::Format::D24S8;
    case D3DFMT_D24X4S4: return nvrhi::Format::D24S8;
    case D3DFMT_D24X8: return nvrhi::Format::D24S8;
    case D3DFMT_D32: return nvrhi::Format::D32;
    case D3DFMT_D32F_LOCKABLE: return nvrhi::Format::D32;
    case D3DFMT_D32_LOCKABLE: return nvrhi::Format::D32;
    case D3DFMT_DF16: return nvrhi::Format::D32;
    case D3DFMT_DF24: return nvrhi::Format::D32;
    case D3DFMT_DXT1: return nvrhi::Format::BC1_UNORM;
    case D3DFMT_DXT2: return nvrhi::Format::BC2_UNORM;
    case D3DFMT_DXT3: return nvrhi::Format::BC2_UNORM;
    case D3DFMT_DXT4: return nvrhi::Format::BC3_UNORM;
    case D3DFMT_DXT5: return nvrhi::Format::BC3_UNORM;
    case D3DFMT_G16R16: return nvrhi::Format::RG16_UNORM;
    case D3DFMT_G16R16F: return nvrhi::Format::RG16_FLOAT;
    case D3DFMT_G32R32F: return nvrhi::Format::RG32_FLOAT;
    case D3DFMT_INDEX16: return nvrhi::Format::R16_UINT;
    case D3DFMT_INDEX32: return nvrhi::Format::R32_UINT;
    case D3DFMT_INTZ: return nvrhi::Format::D24S8;
    case D3DFMT_RAWZ: return nvrhi::Format::D24S8;
    case D3DFMT_L16: return nvrhi::Format::R16_UNORM;
    case D3DFMT_L8: return nvrhi::Format::R8_UNORM;
    case D3DFMT_Q16W16V16U16: return nvrhi::Format::RGBA16_SNORM;
    case D3DFMT_Q8W8V8U8: return nvrhi::Format::RGBA8_SNORM;
    case D3DFMT_R16F: return nvrhi::Format::R16_FLOAT;
    case D3DFMT_R32F: return nvrhi::Format::R32_FLOAT;
    case D3DFMT_R5G6B5: return nvrhi::Format::B5G6R5_UNORM;
    case D3DFMT_R8G8B8: return nvrhi::Format::BGRA8_UNORM;
    case D3DFMT_S8_LOCKABLE: return nvrhi::Format::D24S8;
    case D3DFMT_V16U16: return nvrhi::Format::RG16_SNORM;
    case D3DFMT_V8U8: return nvrhi::Format::RG8_SNORM;
    case D3DFMT_X8R8G8B8: return nvrhi::Format::BGRA8_UNORM;
    default: return nvrhi::Format::UNKNOWN;
    }
}
