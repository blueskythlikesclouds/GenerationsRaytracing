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
    float denominator = (float) reservoir.SampleCount * weight;
    if (denominator > 0.0)
        reservoir.Weight = reservoir.WeightSum * (1.0 / denominator);
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

struct GISample
{
    float3 Color;
    float3 Position;
    float3 Normal;
};

Reservoir<GISample> LoadGIReservoir(
    RWTexture2D<float3> giTexture,
    RWTexture2D<float3> giPositionTexture,
    RWTexture2D<float3> giNormalTexture,
    RWTexture2D<float3> giReservoirTexture,
    uint2 index)
{
    Reservoir<GISample> reservoir;
    reservoir.Sample.Color = giTexture[index];
    reservoir.Sample.Position = giPositionTexture[index];
    reservoir.Sample.Normal = giNormalTexture[index];

    float3 value = giReservoirTexture[index];
    reservoir.WeightSum = value.x;
    reservoir.SampleCount = asuint(value.y);
    reservoir.Weight = value.z;

    return reservoir;
}

void StoreGIReservoir(
    RWTexture2D<float3> giTexture,
    RWTexture2D<float3> giPositionTexture,
    RWTexture2D<float3> giNormalTexture,
    RWTexture2D<float3> giReservoirTexture,
    uint2 index,
    Reservoir<GISample> reservoir)
{
    giTexture[index] = reservoir.Sample.Color;
    giPositionTexture[index] = reservoir.Sample.Position;
    giNormalTexture[index] = reservoir.Sample.Normal;
    giReservoirTexture[index] = float3(reservoir.WeightSum, asfloat(reservoir.SampleCount), reservoir.Weight);
}

float ComputeJacobian(float3 position, float3 normal, float3 neighborPosition, float3 neighborNormal, GISample neighborSample)
{
    float3 offsetB = neighborSample.Position - neighborPosition;
    float3 offsetA = neighborSample.Position - position;
    float RB2 = dot(offsetB, offsetB);
    float RA2 = dot(offsetA, offsetA);
    offsetB = normalize(offsetB);
    offsetA = normalize(offsetA);
    float cosA = dot(normal, offsetA);
    float cosB = dot(neighborNormal, offsetB);
    float cosPhiA = -dot(offsetA, neighborSample.Normal);
    float cosPhiB = -dot(offsetB, neighborSample.Normal);
    float jacobian = RB2 * cosPhiA / (RA2 * cosPhiB);
    return isinf(jacobian) || isnan(jacobian) ? 0.0 : jacobian;

}   


#endif