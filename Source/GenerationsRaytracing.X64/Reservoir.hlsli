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
    RWTexture2D<float4> giTexture,
    RWTexture2D<float4> giPositionTexture,
    RWTexture2D<float4> giNormalTexture,
    uint2 index)
{
    float4 texture = giTexture[index];
    float4 position = giPositionTexture[index];
    float4 normal = giNormalTexture[index];

    Reservoir<GISample> reservoir;

    reservoir.Sample.Color = texture.rgb;
    reservoir.Sample.Position = position.xyz;
    reservoir.Sample.Normal = normal.xyz;

    reservoir.WeightSum = texture.w;
    reservoir.SampleCount = asfloat(position.w);
    reservoir.Weight = normal.w;

    return reservoir;
}

void StoreGIReservoir(
    RWTexture2D<float4> giTexture,
    RWTexture2D<float4> giPositionTexture,
    RWTexture2D<float4> giNormalTexture,
    uint2 index,
    Reservoir<GISample> reservoir)
{
    giTexture[index] = float4(reservoir.Sample.Color, reservoir.WeightSum);
    giPositionTexture[index] = float4(reservoir.Sample.Position, asuint(reservoir.SampleCount));
    giNormalTexture[index] = float4(reservoir.Sample.Normal, reservoir.Weight);
}

float ComputeJacobian(float3 position, float3 neighborPosition, GISample neighborSample)
{
    float dist = distance(position, neighborSample.Position);
    float cosTheta = saturate(dot(normalize(position - neighborSample.Position), neighborSample.Normal));

    float neighborDist = distance(neighborPosition, neighborSample.Position);
    float neighborCosTheta = saturate(dot(normalize(neighborPosition - neighborSample.Position), neighborSample.Normal));

    float jacobian = (cosTheta * neighborDist * neighborDist) / (neighborCosTheta * dist * dist);

    return isinf(jacobian) || isnan(jacobian) ? 0.0 : jacobian;
}   

#endif