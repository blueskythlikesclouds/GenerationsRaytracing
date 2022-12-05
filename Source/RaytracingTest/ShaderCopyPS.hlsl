RWTexture2D<float4> g_Input : register(u0, space0);

float4 main(in float4 position : SV_Position) : SV_Target0
{
    return g_Input[(int2)position.xy];
}