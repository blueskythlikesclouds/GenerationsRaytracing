#ifndef COLOR_SPACE_HLSLI_INCLUDED
#define COLOR_SPACE_HLSLI_INCLUDED

float SrgbToLinear(float value)
{
    return value * (value * (value * 0.305306011 + 0.682171111) + 0.012522878);
}

float3 SrgbToLinear(float3 value)
{
    return float3(SrgbToLinear(value.x), SrgbToLinear(value.y), SrgbToLinear(value.z));
}

float4 SrgbToLinear(float4 value)
{
    return float4(SrgbToLinear(value.rgb), value.a);
}

float LinearToSrgb(float value)
{
    if (value <= 0.0031308)
        return value * 12.92;

    return 1.055 * pow(abs(value), 1.0 / 2.4) - 0.055;
}

float3 LinearToSrgb(float3 value)
{
    return float3(LinearToSrgb(value.x), LinearToSrgb(value.y), LinearToSrgb(value.z));
}

float4 LinearToSrgb(float4 value)
{
    return float4(LinearToSrgb(value.rgb), value.a);
}

#endif