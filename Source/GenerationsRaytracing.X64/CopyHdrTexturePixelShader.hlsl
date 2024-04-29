Texture2D<float4> g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

float4 main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD) : SV_Target
{
    return float4(pow(g_Texture.SampleLevel(g_SamplerState, texCoord, 0).rgb, 2.2), 1.0);
}
