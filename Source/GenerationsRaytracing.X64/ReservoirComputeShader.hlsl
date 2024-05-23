#include "GeometryShading.hlsli"
#include "RootSignature.hlsli"
#include "ThreadGroupTilingX.hlsl"

[numthreads(8, 8, 1)]
void main(uint2 groupThreadId : SV_GroupThreadID, uint2 groupId : SV_GroupID)
{
    uint2 dispatchThreadId = ThreadGroupTilingX(
        (g_InternalResolution + 7) / 8,
        uint2(8, 8),
        8,
        groupThreadId,
        groupId);
    
    GBufferData gBufferData = LoadGBufferData(uint3(dispatchThreadId, 0));
    if (!(gBufferData.Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_LOCAL_LIGHT)))
    {
        uint randSeed = InitRandom(dispatchThreadId);
        float3 eyeDirection = NormalizeSafe(-gBufferData.Position);
    
        // Generate initial candidates
        Reservoir reservoir = (Reservoir) 0;
        uint localLightCount = min(32, g_LocalLightCount);
    
        for (uint i = 0; i < localLightCount; i++)
        {
            uint sample = min(uint(NextRandomFloat(randSeed) * g_LocalLightCount), g_LocalLightCount - 1);
            float weight = ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[sample]) * g_LocalLightCount;
            UpdateReservoir(reservoir, sample, weight, 1, NextRandomFloat(randSeed));
        }
    
        ComputeReservoirWeight(reservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[reservoir.Y]));
    
        g_Reservoir[dispatchThreadId] = StoreReservoir(reservoir);
    }
}