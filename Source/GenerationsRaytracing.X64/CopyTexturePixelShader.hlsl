#include "DebugView.h"

cbuffer Globals : register(b0)
{
    uint2 g_InternalResolution;
    uint g_DebugView;
};

#define EXCLUDE_RAYTRACING_DEFINITIONS
#include "GBufferData.hlsli"

void main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD, 
    out float4 color : SV_Target0, out float depth : SV_Depth)
{
    uint3 index = uint3(texCoord * g_InternalResolution, 0);
    
    switch (g_DebugView)
    {
        case DEBUG_VIEW_NONE:
            color.rgb = min(g_Output_SRV[position.xy], 4.0).rgb;
            break;
        
        case DEBUG_VIEW_DIFFUSE:
            color.rgb = LoadGBufferData(index).Diffuse;
            break;
        
        case DEBUG_VIEW_SPECULAR:
            color.rgb = LoadGBufferData(index).Specular;
            break;
        
        case DEBUG_VIEW_SPECULAR_TINT:
            color.rgb = LoadGBufferData(index).SpecularTint;
            break;
        
        case DEBUG_VIEW_SPECULAR_ENVIRONMENT:
            color.rgb = LoadGBufferData(index).SpecularEnvironment.xxx;
            break;
        
        case DEBUG_VIEW_SPECULAR_GLOSS:
            color.rgb = LoadGBufferData(index).SpecularGloss.xxx / 1024.0;
            break;
        case DEBUG_VIEW_SPECULAR_LEVEL:
            color.rgb = LoadGBufferData(index).SpecularLevel / 5.0;
            break;
        
        case DEBUG_VIEW_SPECULAR_FRESNEL:
            color.rgb = LoadGBufferData(index).SpecularFresnel;
            break;
        
        case DEBUG_VIEW_NORMAL:
            color.rgb = LoadGBufferData(index).Normal * 0.5 + 0.5;
            break;
        
        case DEBUG_VIEW_FALLOFF:
            color.rgb = LoadGBufferData(index).Falloff;
            break;
        
        case DEBUG_VIEW_EMISSION:
            color.rgb = LoadGBufferData(index).Emission;
            break;
        
        case DEBUG_VIEW_TRANS_COLOR:
            color.rgb = LoadGBufferData(index).TransColor;
            break;
        
        case DEBUG_VIEW_REFRACTION:
            color.rgb = LoadGBufferData(index).Refraction.xxx;
            break;
        
        case DEBUG_VIEW_REFRACTION_OVERLAY:
            color.rgb = LoadGBufferData(index).RefractionOverlay.xxx;
            break;
        
        case DEBUG_VIEW_REFRACTION_OFFSET:
            color.rgb = float3(LoadGBufferData(index).RefractionOffset * 0.5 + 0.5, 0.0);
            break;

        case DEBUG_VIEW_GI:
            color.rgb = !(LoadGBufferData(index).Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION)) ? g_GlobalIllumination_SRV[index].rgb : 0.0;
            break;
        
        case DEBUG_VIEW_REFLECTION:
            color.rgb = !(LoadGBufferData(index).Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION)) ? g_Reflection_SRV[index].rgb : 0.0;
            break;
    }
    
    color.a = 1.0;
    depth = g_Depth_SRV.SampleLevel(g_SamplerState, texCoord, 0);
}
