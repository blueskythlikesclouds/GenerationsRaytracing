#ifndef RESERVOIR_H
#define RESERVOIR_H

template<typename T>
struct Reservoir
{
    T Sample;
    float WeightSum;
    uint SampleCount;
    float Weight;
};

template<typename T>
void UpdateReservoir(inout Reservoir<T> reservoir, T sample, float weight, float random)
{
    reservoir.WeightSum += weight;
    reservoir.SampleCount += 1;

    if (random * reservoir.WeightSum < weight)
        reservoir.Sample = sample;
}

template<typename T>
void ComputeReservoirWeight(inout Reservoir<T> reservoir, float weight)
{
    if (weight > 0.0)
        reservoir.Weight = (1.0 / weight) * ((1.0 / reservoir.SampleCount) * reservoir.WeightSum);
    else
        reservoir.Weight = 0.0;
}

Reservoir<uint> LoadDIReservoir(float4 value)
{
    Reservoir<uint> reservoir;
    reservoir.Sample = asuint(value.x);
    reservoir.WeightSum = value.y;
    reservoir.SampleCount = asuint(value.z);
    reservoir.Weight = value.w;

    return reservoir;
}

float4 StoreDIReservoir(Reservoir<uint> reservoir)
{
    float4 value;
    value.x = asfloat(reservoir.Sample);
    value.y = reservoir.WeightSum;
    value.z = asfloat(reservoir.SampleCount);
    value.w = reservoir.Weight;

    return value;
}

#endif