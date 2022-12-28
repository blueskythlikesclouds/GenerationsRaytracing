cbuffer cbGlobals : register(b0)
{
    float4 g_ViewportSize : packoffset(c180);
};

void main(in float4 vPosition : POSITION, in float4 vColor : COLOR, in float4 vTexCoord : TEXCOORD,
    out float4 oPosition : SV_Position,
    out float4 oTexCoord : TEXCOORD,
    out float4 oTexCoord1 : TEXCOORD1,
    out float4 oTexCoord2 : TEXCOORD2,
    out float4 oTexCoord3 : TEXCOORD3,
    out float4 oTexCoord4 : TEXCOORD4,
    out float4 oTexCoord5 : TEXCOORD5,
    out float4 oTexCoord6 : TEXCOORD6,
    out float4 oTexCoord7 : TEXCOORD7,
    out float4 oTexCoord8 : TEXCOORD8,
    out float4 oTexCoord9 : TEXCOORD9,
    out float4 oTexCoord10 : TEXCOORD10,
    out float4 oTexCoord11 : TEXCOORD11,
    out float4 oTexCoord12 : TEXCOORD12,
    out float4 oTexCoord13 : TEXCOORD13,
    out float4 oTexCoord14 : TEXCOORD14,
    out float4 oTexCoord15 : TEXCOORD15,
    out float4 oColor : COLOR,
    out float4 oColor1 : COLOR1)
{
    oPosition = float4((vPosition.xy + 0.5) * g_ViewportSize.zw * float2(2, -2) + float2(-1, 1), vPosition.zw);
    oTexCoord = vTexCoord;
    oTexCoord1 = 0;
    oTexCoord2 = 0;
    oTexCoord3 = 0;
    oTexCoord4 = 0;
    oTexCoord5 = 0;
    oTexCoord6 = 0;
    oTexCoord7 = 0;
    oTexCoord8 = 0;
    oTexCoord9 = 0;
    oTexCoord10 = 0;
    oTexCoord11 = 0;
    oTexCoord12 = 0;
    oTexCoord13 = 0;
    oTexCoord14 = 0;
    oTexCoord15 = 0;
    oColor = vColor;
    oColor1 = 0;
}