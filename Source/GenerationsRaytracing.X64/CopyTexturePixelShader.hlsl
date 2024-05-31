#include "DebugView.h"

cbuffer Globals : register(b0)
{
    uint2 g_InternalResolution;
    uint g_DebugView;
};

#define EXCLUDE_RAYTRACING_DEFINITIONS
#include "GBufferData.hlsli"

#define MAKE_DEBUG_VIEW(VALUE) \
    do { \
        uint layerNum = g_LayerNum_SRV[index.xy]; \
        color.rgb = 0.0; \
        for (uint i = 0; i < layerNum; i++) \
        { \
            index.z = i; \
            float3 value = VALUE; \
            float alpha = LoadGBufferData(index).Alpha; \
            if (!(LoadGBufferData(index).Flags & GBUFFER_FLAG_IS_ADDITIVE)) \
            { \
                color.rgb *= 1.0 - alpha; \
            } \
            color.rgb += value * alpha; \
        } \
    } while (0)

void main(in float4 position : SV_Position, in float2 texCoord : TEXCOORD, 
    out float4 color : SV_Target0, out float depth : SV_Depth)
{
    uint3 index = uint3(texCoord * g_InternalResolution, 0);
    
    switch (g_DebugView)
    {
        case DEBUG_VIEW_NONE:
            color.rgb = min(g_Output_SRV[position.xy], 4.0).rgb;
            break;
        
        case DEBUG_VIEW_COLOR:
            color.rgb = g_Color_SRV[index.xy].rgb;
            break;
        
        case DEBUG_VIEW_DEPTH:
            color.rgb = g_Depth_SRV[index.xy];
            break;
        
        case DEBUG_VIEW_MOTION_VECTORS:
            color.rgb = float3((g_MotionVectors_SRV[index.xy] / g_InternalResolution) * 0.5 + 0.5, 1.0);
            break;
        
        case DEBUG_VIEW_DIFFUSE:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Diffuse);
            break;
        
        case DEBUG_VIEW_SPECULAR:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Specular);
            break;
        
        case DEBUG_VIEW_SPECULAR_TINT:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).SpecularTint);
            break;
        
        case DEBUG_VIEW_SPECULAR_ENVIRONMENT:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).SpecularEnvironment.xxx);
            break;
        
        case DEBUG_VIEW_SPECULAR_GLOSS:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).SpecularGloss.xxx / 1024.0);
            break;
        case DEBUG_VIEW_SPECULAR_LEVEL:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).SpecularLevel / 5.0);
            break;
        
        case DEBUG_VIEW_SPECULAR_FRESNEL:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).SpecularFresnel);
            break;
        
        case DEBUG_VIEW_NORMAL:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Normal * 0.5 + 0.5);
            break;
        
        case DEBUG_VIEW_FALLOFF:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Falloff);
            break;
        
        case DEBUG_VIEW_EMISSION:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Emission);
            break;
        
        case DEBUG_VIEW_TRANS_COLOR:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).TransColor);
            break;
        
        case DEBUG_VIEW_REFRACTION:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).Refraction.xxx);
            break;
        
        case DEBUG_VIEW_REFRACTION_OVERLAY:
            MAKE_DEBUG_VIEW(LoadGBufferData(index).RefractionOverlay.xxx);
            break;
        
        case DEBUG_VIEW_REFRACTION_OFFSET:
            MAKE_DEBUG_VIEW(float3(LoadGBufferData(index).RefractionOffset * 0.5 + 0.5, 1.0));
            break;

        case DEBUG_VIEW_GI:
            MAKE_DEBUG_VIEW(!(LoadGBufferData(index).Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_GLOBAL_ILLUMINATION)) ? g_GlobalIllumination_SRV[index].rgb : 0.0);
            break;
        
        case DEBUG_VIEW_REFLECTION:
            MAKE_DEBUG_VIEW(!(LoadGBufferData(index).Flags & (GBUFFER_FLAG_IS_SKY | GBUFFER_FLAG_IGNORE_REFLECTION)) ? g_Reflection_SRV[index].rgb : 0.0);
            break;
    }
    
    color.a = 1.0;
    depth = g_Depth_SRV.SampleLevel(g_SamplerState, texCoord, 0);
}
