cbuffer cbGlobals : register(b0)
{
    bool g_IsEnableBoostBlur;
};

Texture2D<float4> g_Texture : register(t0);
SamplerState g_PointClampSampler : register(s0);

void main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD, out float4 color : SV_Target0)
{
    uint width;
    uint height;
    g_Texture.GetDimensions(width, height);

    float4 texture = g_Texture.SampleLevel(g_PointClampSampler, texCoord, 0);
    float2 velocity = g_IsEnableBoostBlur ? texture.xy : texture.zw;

    velocity.x /= width;
    velocity.y /= height;

    velocity *= 0.5;
    velocity += 0.5;

    velocity *= 255;

    color.y = frac(velocity.x);
    color.x = (velocity.x - color.y) / 255;

    color.w = frac(velocity.y);
    color.z = (velocity.y - color.w) / 255;
}