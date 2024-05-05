#include "PackedPrimitives.hlsli"

StructuredBuffer<float4x4> g_Pose : register(t0);

cbuffer GeometryDesc : register(b0)
{
    uint g_VertexCount;
    uint g_VertexStride;
    uint g_NormalOffset;
    uint g_TangentOffset;
    uint g_BinormalOffset;
    uint g_BlendWeightOffset;
    uint g_BlendIndicesOffset;
    uint g_BlendWeight1Offset;
    uint g_BlendIndices1Offset;
    uint g_NodeCount;
    bool g_Visible;
}

StructuredBuffer<uint> g_Palette : register(t1);
ByteAddressBuffer g_Src : register(t2);
RWByteAddressBuffer g_Dest : register(u0);

uint4 UnpackUint4(uint value)
{
    return uint4(
        (value >> 0) & 0xFF,
        (value >> 8) & 0xFF,
        (value >> 16) & 0xFF,
        (value >> 24) & 0xFF);
}

float4x4 ComputeNodeMatrix(uint4 blendIndices, float4 blendWeight)
{
    return
        g_Pose[g_Palette[blendIndices.x]] * blendWeight.x +
        g_Pose[g_Palette[blendIndices.y]] * blendWeight.y +
        g_Pose[g_Palette[blendIndices.z]] * blendWeight.z +
        g_Pose[g_Palette[blendIndices.w]] * blendWeight.w;
}

bool CheckNodeMatrixZeroScaled(float4x4 nodeMatrix)
{
    return dot(nodeMatrix[0].xyz, nodeMatrix[0].xyz) == 0 &&
        dot(nodeMatrix[1].xyz, nodeMatrix[1].xyz) == 0 &&
        dot(nodeMatrix[2].xyz, nodeMatrix[2].xyz) == 0;
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
    if (dispatchThreadId.x >= g_VertexCount)
        return;

    uint vertexOffset = dispatchThreadId.x * g_VertexStride;
    uint prevVertexOffset = g_VertexCount * g_VertexStride + dispatchThreadId.x * 0xC;
    
    float3 position = float3(asfloat(0x7FFFFFFF), 0.0, 0.0);
    uint3 prevPosition = g_Dest.Load3(vertexOffset);
    
    if (g_Visible)
    {
        uint2 blendIndicesAndWeight = g_Src.Load2(vertexOffset + g_BlendIndicesOffset);
        uint4 blendIndices = min(g_NodeCount - 1, UnpackUint4(blendIndicesAndWeight.x));
        
        float4 blendWeight = UnpackUint4(blendIndicesAndWeight.y) / 255.0;
        float weightSum = blendWeight.x + blendWeight.y + blendWeight.z + blendWeight.w;
        
        float4x4 nodeMatrix = 0.0;
        
        if (g_BlendIndices1Offset != 0)
        {
            uint2 blendIndicesAndWeight1 = g_Src.Load2(vertexOffset + g_BlendIndices1Offset);
            uint4 blendIndices1 = min(g_NodeCount - 1, UnpackUint4(blendIndicesAndWeight1.x));
            
            float4 blendWeight1 = UnpackUint4(blendIndicesAndWeight1.y) / 255.0;
            weightSum += blendWeight1.x + blendWeight1.y + blendWeight1.z + blendWeight1.w;
            
            if (weightSum > 0.0)
                blendWeight1 /= weightSum;
            
            nodeMatrix += ComputeNodeMatrix(blendIndices1, blendWeight1);
        }
        
        if (weightSum > 0.0)
            blendWeight /= weightSum;
        else
            blendWeight.w = 1.0;

        nodeMatrix += ComputeNodeMatrix(blendIndices, blendWeight);
        
        if (!CheckNodeMatrixZeroScaled(nodeMatrix))
        {
            position = g_Src.Load<float3>(vertexOffset);
            uint3 nrmTgnBin = g_Src.Load3(vertexOffset + g_NormalOffset);
            float3 normal = DecodeSnorm10(nrmTgnBin.x);
            float3 tangent = DecodeSnorm10(nrmTgnBin.y);
            float3 binormal = DecodeSnorm10(nrmTgnBin.z);

            position = mul(nodeMatrix, float4(position, 1.0)).xyz;
            normal = normalize(mul(nodeMatrix, float4(normal, 0.0)).xyz);
            tangent = normalize(mul(nodeMatrix, float4(tangent, 0.0)).xyz);
            binormal = normalize(mul(nodeMatrix, float4(binormal, 0.0)).xyz);

            g_Dest.Store3(vertexOffset + g_NormalOffset, uint3(EncodeSnorm10(normal), EncodeSnorm10(tangent), EncodeSnorm10(binormal)));
        }
    }
    
    g_Dest.Store<float3>(vertexOffset, position);
    g_Dest.Store3(prevVertexOffset, isnan(asfloat(prevPosition.x)) ? asuint(position) : prevPosition);
}