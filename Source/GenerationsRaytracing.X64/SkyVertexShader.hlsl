#include "SkySharedDefinitions.hlsli"

float2 GetTexCoord(VertexShaderInput i, uint index)
{
    switch (index)
    {
        case 0:
        default:
            return i.TexCoord;
        case 1:
            return i.TexCoord1;
        case 2:
            return i.TexCoord2;
        case 3:
            return i.TexCoord3;
    }
}

void main(in VertexShaderInput i, out PixelShaderInput o)
{
    o.Position = mul(float4(mul(float4(i.Position, 0.0), g_MtxView).xyz, 1.0), g_MtxProjection);
    o.DiffuseTexCoord = GetTexCoord(i, g_DiffuseTextureId >> 30);
    o.AlphaTexCoord = GetTexCoord(i, g_AlphaTextureId >> 30);
    o.EmissionTexCoord = GetTexCoord(i, g_EmissionTextureId >> 30);
    o.Color = i.Color;
    o.Color.rgb *= g_BackGroundScale;
}
