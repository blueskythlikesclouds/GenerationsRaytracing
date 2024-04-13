#pragma once

#define ANTI_ALIASING 2.0

cbuffer Globals : register(b0)
{
    row_major float4x4 g_MtxProjection : packoffset(c0);
    row_major float4x4 g_MtxView : packoffset(c4);
    float2 g_ViewportSize : packoffset(c8);
};

struct VertexData
{
    float3 Position;
    float Size;
    uint Color;
};

StructuredBuffer<VertexData> g_VertexData : register(t0);

float2 ComputeVertex(uint vertexId)
{
    return float2((vertexId & 1) != 0 ? 1.0 : -1.0, (vertexId & 2) != 0 ? 1.0 : -1.0);
}

float4 DecodeColor(uint color)
{
    float4 result;
    result.r = float((color & 0xff000000u) >> 24u) / 255.0;
    result.g = float((color & 0x00ff0000u) >> 16u) / 255.0;
    result.b = float((color & 0x0000ff00u) >> 8u) / 255.0;
    result.a = float((color & 0x000000ffu) >> 0u) / 255.0;
    return result;
}