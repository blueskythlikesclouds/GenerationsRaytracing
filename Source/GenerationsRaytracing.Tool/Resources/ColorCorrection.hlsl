cbuffer cbGlobalsPS : register(b1)
{
    // x = params.brightness
    // y = params.contrast
    // z = params.hue
    // w = params.saturation
    float4 cY : packoffset(c150);
}

Texture2D<float4> s0 : register(t0);
SamplerState s0_s : register(s0);

#define EPSILON 1e-10

float3 hue2rgb(in float H)
{
	float r = abs(H * 6 - 3) - 1;
	float g = 2 - abs(H * 6 - 2);
	float b = 2 - abs(H * 6 - 4);
	return saturate(float3(r, g, b));
}

float3 rgb2ybr(in float3 rgb)
{
	float y = dot(rgb, float3(0.299, 0.587, 0.114));
	float cb = dot(rgb, float3(-0.168736, -0.331264, 0.5));
	float cr = dot(rgb, float3(0.5, -0.418688, -0.081312));

	return float3(y, cb, cr);
}

float3 rgb2hcv(in float3 rgb)
{
    float4 p = (rgb.g < rgb.b) ? float4(rgb.bg, -1.0, 2.0 / 3.0) : float4(rgb.gb, 0.0, -1.0 / 3.0);
    float4 q = (rgb.r < p.x) ? float4(p.xyw, rgb.r) : float4(rgb.r, p.yzx);
    float c = q.x - min(q.w, q.y);
    float h = abs((q.w - q.y) / (6 * c + EPSILON) + q.z);
    return float3(h, c, q.x);
}

float3 rgb2hsv(in float3 rgb)
{
    float3 hcv = rgb2hcv(rgb);
    float s = hcv.y / (hcv.z + EPSILON);
    return float3(hcv.x, s, hcv.z);
}

float3 hsv2rgb(in float3 hsv)
{
    float3 rgb = hue2rgb(hsv.x);
    return ((rgb - 1) * hsv.y + 1) * hsv.z;
}

float3 ybr2rgb(in float3 ybr)
{
	float r = dot(ybr, float3(1.0, -0.0000012188942, 1.4019995886573403572));
	float g = dot(ybr, float3(1.0, -0.3441356781654, -0.71413615558181249872));
	float b = dot(ybr, float3(1.0, 1.7720000660738161801, 0.00000040629806289390168897));

	return float3(r, g, b);
}

float4 main(in float4 unused : SV_POSITION, in float4 texCoord : TEXCOORD0) : SV_TARGET
{
	float3 ybr = rgb2ybr(saturate(s0.Sample(s0_s, texCoord.xy).rgb));
	ybr.x = max(-1.0, min(1.0, (ybr.x - 0.5) * cY.y + cY.x + 0.5));

	float3 hsv = rgb2hsv(ybr2rgb(ybr));
	hsv.x = hsv.x + cY.z / 360.0;
	hsv.x = hsv.x < 0.0 ? 1.0 - frac(-hsv.x) : frac(hsv.x);
	hsv.y = saturate(hsv.y * cY.w * cY.w);

	return float4(saturate(hsv2rgb(hsv)), 1.0);
}