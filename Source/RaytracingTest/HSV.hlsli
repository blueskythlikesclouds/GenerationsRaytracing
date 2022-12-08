#ifndef HSV_H
#define HSV_H

#define EPSILON 1e-10

float3 hue2rgb(in float H)
{
    float r = abs(H * 6 - 3) - 1;
    float g = 2 - abs(H * 6 - 2);
    float b = 2 - abs(H * 6 - 4);
    return saturate(float3(r, g, b));
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

#endif
