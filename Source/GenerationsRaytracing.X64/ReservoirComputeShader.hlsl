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
    
        // Temporal reuse
        if (g_CurrentFrame > 0)
        {
            int2 prevIndex = (float2) dispatchThreadId - g_PixelJitter + 0.5 + g_MotionVectors_SRV[dispatchThreadId];
    
            // TODO: Check for previous normal and depth
    
            Reservoir prevReservoir = LoadReservoir(g_PrevReservoir[prevIndex]);
            prevReservoir.M = min(reservoir.M * 20, prevReservoir.M);
    
            Reservoir temporalReservoir = (Reservoir) 0;
    
            UpdateReservoir(temporalReservoir, reservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                g_LocalLights[reservoir.Y]) * reservoir.W * reservoir.M, reservoir.M, NextRandomFloat(randSeed));
    
            UpdateReservoir(temporalReservoir, prevReservoir.Y, ComputeReservoirWeight(gBufferData, eyeDirection, 
                g_LocalLights[prevReservoir.Y]) * prevReservoir.W * prevReservoir.M, prevReservoir.M, NextRandomFloat(randSeed));
    
            ComputeReservoirWeight(temporalReservoir, ComputeReservoirWeight(gBufferData, eyeDirection, g_LocalLights[temporalReservoir.Y]));
    
            reservoir = temporalReservoir;
        }
    
        g_Reservoir[dispatchThreadId] = StoreReservoir(reservoir);
    }
}