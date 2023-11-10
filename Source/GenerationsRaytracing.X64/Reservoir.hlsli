#ifndef RESERVOIR_HLSLI_INCLUDED
#define RESERVOIR_HLSLI_INCLUDED

struct Reservoir
{
    uint Y;
    float WSum;
    uint M;
    float W;
};

void UpdateReservoir(inout Reservoir reservoir, uint sample, float weight, float random)
{
    reservoir.WSum += weight;
    reservoir.M += 1;

    if (random * reservoir.WSum < weight)
        reservoir.Y = sample;
}

void ComputeReservoirWeight(inout Reservoir reservoir, float weight)
{
    float denominator = reservoir.M * weight;
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

#endif