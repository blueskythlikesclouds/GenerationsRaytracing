#ifndef RESERVOIR_H
#define RESERVOIR_H

struct Reservoir
{
    uint Y;
    float WSum;
    uint M;
    float W;
};

void UpdateReservoir(inout Reservoir r, uint y, float w, float random)
{
    r.WSum += w;
    r.M += 1;

    if (random < (w / r.WSum))
        r.Y = y;
}

void ComputeReservoirWeight(inout Reservoir r, float w)
{
    if (w > 0.0)
        r.W = (1.0 / w) * ((1.0 / r.M) * r.WSum);
    else
        r.W = 0.0;
}

Reservoir LoadReservoir(float4 value)
{
    Reservoir r;
    r.Y = asuint(value.x);
    r.WSum = value.y;
    r.M = asuint(value.z);
    r.W = value.w;

    return r;
}

float4 StoreReservoir(Reservoir r)
{
    float4 value;
    value.x = asfloat(r.Y);
    value.y = r.WSum;
    value.z = asfloat(r.M);
    value.w = r.W;

    return value;
}

#endif