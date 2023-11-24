#ifndef PACKED_PRIMITIVES_HLSLI_INCLUDED
#define PACKED_PRIMITIVES_HLSLI_INCLUDED

float4 DecodeColor(uint value)
{
    return float4(
        ((value >> 0) & 0xFF) / 255.0,
        ((value >> 8) & 0xFF) / 255.0,
        ((value >> 16) & 0xFF) / 255.0,
        ((value >> 24) & 0xFF) / 255.0);
}

float2 DecodeFloat2(uint value)
{
    return float2(f16tof32(value), f16tof32(value >> 16));
}

uint EncodeFloat2(float2 value)
{
    return f32tof16(value.x) | (f32tof16(value.y) << 16);
}

float3 DecodeFloat3(uint2 value)
{
    return float3(f16tof32(value.x), f16tof32(value.x >> 16), f16tof32(value.y));
}

uint2 EncodeFloat3(float3 value)
{
    return uint2(f32tof16(value.x) | (f32tof16(value.y) << 16), f32tof16(value.z));
}

float4 DecodeFloat4(uint2 value)
{
    return float4(f16tof32(value.x), f16tof32(value.x >> 16), f16tof32(value.y), f16tof32(value.y >> 16));
}

uint2 EncodeFloat4(float4 value)
{
    return uint2(f32tof16(value.x) | (f32tof16(value.y) << 16), f32tof16(value.z) | (f32tof16(value.w) << 16));
}

#endif