#ifndef RESERVOIR_H
#define RESERVOIR_H

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
}

Reservoir LoadReservoir(float4 value)
{
    Reservoir reservoir;
    reservoir.Y = asuint(value.x);
    reservoir.WSum = value.y;
    reservoir.M = asuint(value.z);
    reservoir.W = value.w;

    return reservoir;
}

float4 StoreReservoir(Reservoir reservoir)
{
    float4 value;
    value.x = asfloat(reservoir.Y);
    value.y = reservoir.WSum;
    value.z = asfloat(reservoir.M);
    value.w = reservoir.W;

    return value;
}

#endif