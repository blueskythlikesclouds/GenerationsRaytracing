Texture2D<float4> g_Texture : register(t0);
Texture2D<float> g_Depth : register(t1);

SamplerState g_PointClampSampler : register(s0);

void main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD, out float4 color : SV_Target0, out float depth : SV_Depth)
{
    color = min(g_Texture.SampleLevel(g_PointClampSampler, texCoord, 0), 4.0);
    depth = g_Depth.SampleLevel(g_PointClampSampler, texCoord, 0);
}