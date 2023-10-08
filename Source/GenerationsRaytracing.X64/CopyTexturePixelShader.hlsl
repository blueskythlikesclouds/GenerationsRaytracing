RWTexture2D<float4> g_Texture : register(u0);
RWTexture2D<float> g_Depth : register(u1);

void main(in float4 position : SV_Position, out float4 color : SV_Target0, out float depth : SV_Depth)
{
    color = min(4.0, g_Texture[position.xy]);
    depth = g_Depth[position.xy];
}