Texture2D<float3> g_ColorTexture : register(t0);
Texture2D<float> g_DepthTexture : register(t1);
SamplerState g_SamplerState : register(s0);

void main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD, 
    out float4 color : SV_Target0, out float depth : SV_Depth)
{
    color = float4(min(g_ColorTexture.SampleLevel(g_SamplerState, texCoord, 0), 4.0), 1.0);
    depth = g_DepthTexture.SampleLevel(g_SamplerState, texCoord, 0);
}
