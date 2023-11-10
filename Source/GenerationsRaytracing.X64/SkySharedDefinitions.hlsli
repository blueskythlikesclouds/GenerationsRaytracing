#ifndef SKY_SHARED_DEFINITIONS_HLSLI_INCLUDED
#define SKY_SHARED_DEFINITIONS_HLSLI_INCLUDED

cbuffer GlobalsSB : register(b0)
{
    float g_BackGroundScale;
    uint g_DiffuseTextureId;
    uint g_AlphaTextureId;
    uint g_EmissionTextureId;
    float3 g_Ambient;
    bool g_EnableAlphaTest;
}

struct VertexShaderInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float2 TexCoord1 : TEXCOORD1;
    float2 TexCoord2 : TEXCOORD2;
    float2 TexCoord3 : TEXCOORD3;
    float4 Color : COLOR;
    uint InstanceID : SV_InstanceID;
};

struct PixelShaderInput
{
    float2 DiffuseTexCoord : TEXCOORD;
    float2 AlphaTexCoord : TEXCOORD1;
    float2 EmissionTexCoord : TEXCOORD2;
    float4 Color : COLOR;
    float4 Position : SV_Position;
    uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
};

#endif