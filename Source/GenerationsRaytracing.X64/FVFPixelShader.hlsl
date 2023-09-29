cbuffer cbGlobalsPS : register(b1)
{
    uint s0_tI : packoffset(c224.x);
    uint s0_sI : packoffset(c240.x);
}

float4 main(
    in float4 position : SV_Position,
    in float4 texCoord : TEXCOORD,
    in float4 texCoord1 : TEXCOORD1,
    in float4 texCoord2 : TEXCOORD2,
    in float4 texCoord3 : TEXCOORD3,
    in float4 texCoord4 : TEXCOORD4,
    in float4 texCoord5 : TEXCOORD5,
    in float4 texCoord6 : TEXCOORD6,
    in float4 texCoord7 : TEXCOORD7,
    in float4 texCoord8 : TEXCOORD8,
    in float4 texCoord9 : TEXCOORD9,
    in float4 texCoord10 : TEXCOORD10,
    in float4 texCoord11 : TEXCOORD11,
    in float4 texCoord12 : TEXCOORD12,
    in float4 texCoord13 : TEXCOORD13,
    in float4 texCoord14 : TEXCOORD14,
    in float4 texCoord15 : TEXCOORD15,
    in float4 color : COLOR,
    in float4 color1 : COLOR1) : SV_Target0
{
    Texture2D<float4> s0 = ResourceDescriptorHeap[s0_tI];
    SamplerState s0_s = SamplerDescriptorHeap[s0_sI];

    return s0.Sample(s0_s, texCoord.xy) * color;
}