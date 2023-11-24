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

float2 DecodeTexCoord(uint value)
{
    return float2(f16tof32(value), f16tof32(value >> 16));
}

uint EncodeTexCoord(float2 value)
{
    return f32tof16(value.x) | (f32tof16(value.y) << 16);
}

float3 DecodeNormal(uint2 value)
{
    return float3(f16tof32(value.x), f16tof32(value.x >> 16), f16tof32(value.y));
}

uint2 EncodeNormal(float3 value)
{
    return uint2(f32tof16(value.x) | (f32tof16(value.y) << 16), f32tof16(value.z));
}

#endif