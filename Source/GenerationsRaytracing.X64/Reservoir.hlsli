#pragma once

struct Reservoir
{
    uint Y;
    float WSum;
    uint M;
    float W;
};

void UpdateReservoir(inout Reservoir reservoir, uint y, float w, uint c, float random)
{
    reservoir.WSum += w;
    reservoir.M += c;

    if (random * reservoir.WSum < w)
        reservoir.Y = y;
}

void ComputeReservoirWeight(inout Reservoir reservoir, float w)
{
    float denominator = reservoir.M * w;
    if (denominator > 0.0)
        reservoir.W = reservoir.WSum / denominator;
    else
        reservoir.W = 0.0;
}

Reservoir LoadReservoir(uint4 value)
{
    Reservoir reservoir;
    reservoir.Y = value.x;
    reservoir.WSum = asfloat(value.y);
    reservoir.M = value.z;
    reservoir.W = asfloat(value.w);

    return reservoir;
}

uint4 StoreReservoir(Reservoir reservoir)
{
    uint4 value;
    value.x = reservoir.Y;
    value.y = asuint(reservoir.WSum);
    value.z = reservoir.M;
    value.w = asuint(reservoir.W);

    return value;
}