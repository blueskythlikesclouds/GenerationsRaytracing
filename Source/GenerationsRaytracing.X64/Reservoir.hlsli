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

    if (random < (weight / reservoir.WeightSum))
        reservoir.Sample = sample;
}

template<typename T>
void ComputeReservoirWeight(inout Reservoir<T> reservoir, float weight)
{
    reservoir.Weight = reservoir.WeightSum / max(0.0001, (float) reservoir.SampleCount * weight);
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

struct GISample
{
    float3 Color;
    float3 Position;
};

Reservoir<GISample> LoadGIReservoir(
    RWTexture2D<float3> giTexture,
    RWTexture2D<float3> giPositionTexture,
    RWTexture2D<float3> giReservoirTexture,
    uint2 index)
{
    Reservoir<GISample> reservoir;
    reservoir.Sample.Color = giTexture[index];
    reservoir.Sample.Position = giPositionTexture[index];

    float3 value = giReservoirTexture[index];
    reservoir.WeightSum = value.x;
    reservoir.SampleCount = asuint(value.y);
    reservoir.Weight = value.z;

    return reservoir;
}

void StoreGIReservoir(
    RWTexture2D<float3> giTexture,
    RWTexture2D<float3> giPositionTexture,
    RWTexture2D<float3> giReservoirTexture,
    uint2 index,
    Reservoir<GISample> reservoir)
{
    giTexture[index] = reservoir.Sample.Color;
    giPositionTexture[index] = reservoir.Sample.Position;
    giReservoirTexture[index] = float3(reservoir.WeightSum, asfloat(reservoir.SampleCount), reservoir.Weight);
}

#endif