#ifndef SHADER_TONE_MAP_H
#define SHADER_TONE_MAP_H

#include "ShaderDefinitions.hlsli"

Texture2D<float4> g_Texture0 : register(t0, space0);
Texture2D<float4> g_Texture1 : register(t1, space0);
SamplerState g_LinearClampSampler : register(s0, space0);

#endif