#pragma once

float4 DecodeColor(uint value)
{
    return float4(
        ((value >> 0) & 0xFF) / 255.0,
        ((value >> 8) & 0xFF) / 255.0,
        ((value >> 16) & 0xFF) / 255.0,
        ((value >> 24) & 0xFF) / 255.0);
}

float2 DecodeHalf2(uint value)
{
    return float2(f16tof32(value), f16tof32(value >> 16));
}

uint EncodeHalf2(float2 value)
{
    return f32tof16(value.x) | (f32tof16(value.y) << 16);
}

float3 DecodeHalf3(uint2 value)
{
    return float3(f16tof32(value.x), f16tof32(value.x >> 16), f16tof32(value.y));
}

uint2 EncodeHalf3(float3 value)
{
    return uint2(f32tof16(value.x) | (f32tof16(value.y) << 16), f32tof16(value.z));
}

float4 DecodeHalf4(uint2 value)
{
    return float4(f16tof32(value.x), f16tof32(value.x >> 16), f16tof32(value.y), f16tof32(value.y >> 16));
}

uint2 EncodeHalf4(float4 value)
{
    return uint2(f32tof16(value.x) | (f32tof16(value.y) << 16), f32tof16(value.z) | (f32tof16(value.w) << 16));
}

float3 DecodeUnorm10(uint value)
{
    return float3(value & 0x3FF, (value >> 10) & 0x3FF, (value >> 20) & 0x3FF) / 1023.0;
}

float3 DecodeSnorm10(uint value)
{
    return DecodeUnorm10(value) * 2.0 - 1.0;
}

uint EncodeUnorm(float v, uint N)
{
    const float scale = float((1u << N) - 1);

    v = (v >= 0) ? v : 0;
    v = (v <= 1) ? v : 1;

    return uint(v * scale + 0.5);
}

uint EncodeUnorm10(float3 value)
{
    return (EncodeUnorm(value.x, 10) & 0x3FF) | ((EncodeUnorm(value.y, 10) & 0x3FF) << 10) | ((EncodeUnorm(value.z, 10) & 0x3FF) << 20);
}

uint EncodeSnorm10(float3 value)
{
    return EncodeUnorm10(value * 0.5 + 0.5);
}