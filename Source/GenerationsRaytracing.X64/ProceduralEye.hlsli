#pragma once

#include "SharedDefinitions.hlsli"

float2 GetOvalCoords(float2 texCoord, float2 scale)
{
    return (texCoord * 2.0 - 1.0) * scale;
}

float MakeGradient(float start, float end, float position, float transitionPosition, float transitionSharpness)
{
    return lerp(start, end, smoothstep(0.0, 1.0, saturate((position - transitionPosition) * transitionSharpness)));
}

float3 MakeGradient(float3 start, float3 end, float position, float transitionPosition, float transitionSharpness)
{
    return lerp(start, end, smoothstep(0.0, 1.0, saturate((position - transitionPosition) * transitionSharpness)));
}

void GenerateProceduralEye(
    in float2 irisTexCoord, 
    in float3 irisColor, 
    in float2 pupilTexCoord,
	in float2 pupilScale,
	in float pupilTransitionPosition,
	in float pupilTransitionStrength,
    in float2 highLightTexCoord,
    in float3 highLightColor,
    in float2 catchLightTexCoord,
    out float3 diffuse,
    out float pupil, 
    out float3 highLight,
    out float catchLight,
    out float4 mask)
{
    // Iris
    const float2 irisScale = float2(1.1, 0.5);
    float2 irisOvalCoords = GetOvalCoords(irisTexCoord, irisScale);
    float2 irisOvalCenter = GetOvalCoords(0.5 + irisTexCoord - pupilTexCoord, irisScale);
    float irisRadius = length(irisOvalCoords);
    float parallaxFactor = irisRadius / distance(irisOvalCoords, irisOvalCenter);
    parallaxFactor *= parallaxFactor;
    float irisPosition = saturate((1.0 - irisRadius / 0.17) * parallaxFactor);

    float3 irisGradient = MakeGradient(irisColor * 0.25, irisColor * 0.5, irisPosition, 0.015, 16.0);
    irisGradient = MakeGradient(irisGradient, irisColor * 0.75, irisPosition, 0.04, 8.0);
    irisGradient = MakeGradient(irisGradient, irisColor, irisPosition, 0.15, 8.0);

    // Iris Line
    const float irisLineSegments = 128.0;
    float irisAngle = atan2(irisOvalCoords.y, irisOvalCoords.x) / PI * 0.5 + 0.5;
    uint irisLineRandSeed = uint(irisAngle * irisLineSegments);
    float irisLine = MakeGradient(0.85, 1.0, irisPosition, NextRandomFloat(irisLineRandSeed) * 0.125, 4.0);
    irisLine = MakeGradient(1.0, irisLine, irisPosition, NextRandomFloat(irisLineRandSeed) * 0.0625, 8.0);

    float irisLineFade = frac(irisAngle * irisLineSegments);
    irisLineFade = irisLineFade <= 0.5 ? irisLineFade * 2.0 : (1.0 - irisLineFade) * 2.0;
    irisLine = lerp(1.0, irisLine, smoothstep(0.0, 1.0, irisLineFade));
	
    irisGradient *= irisLine;

    // Sclera
    float scleraPosition = saturate((irisRadius - 0.17) / (sqrt(2.0) - 0.17));
    float3 scleraColor = 0.82353;
    float3 scleraShadeColor = float3(0.62, 0.67, 0.68);
    float3 scleraGradient = MakeGradient(scleraShadeColor, scleraColor, scleraPosition, -0.027, 16.0);

    diffuse = irisRadius > 0.17 ? scleraGradient : irisGradient;
    
    // Pupil
    float2 pupilOvalCoords = GetOvalCoords(pupilTexCoord.xy, pupilScale);
    float pupilRadius = length(pupilOvalCoords);
    pupil = 1.0 - MakeGradient(0.0, 1.0, saturate(1.0 - pupilRadius / 0.1), pupilTransitionPosition, pupilTransitionStrength);
    
    // Highlight
    float2 highLightOvalCoords = GetOvalCoords(highLightTexCoord.xy, float2(1.0, 0.95));
    float highLightRadius = length(highLightOvalCoords);
    float highLightPosition = saturate(highLightRadius / 0.3);
    highLight = MakeGradient(highLightColor, 0.0, highLightPosition, 0.3, 3.0);
    
    // Catchlight
    const float2 catchLightScale = float2(0.8, 0.5);
    float2 catchLightOvalCoords = GetOvalCoords(catchLightTexCoord.xy, catchLightScale);
    float catchLightRadius = length(catchLightOvalCoords);
    catchLight = catchLightRadius < 0.032;
    
    // Mask
    mask.x = 0.0;
    mask.y = 0.0;
    
    mask.z = irisRadius > 0.17 ? 1.0 : MakeGradient(0.17, 0.98, irisPosition, -0.05, 2.1);
    
    mask.w = irisRadius > 0.17 ? 0.0 : MakeGradient(0.3, 1.0, irisPosition, 0.0, 8.0);
    mask.w *= irisLine;
    mask.w *= irisLine;
    float pupilPosition = saturate(pupilRadius / 0.13);
    mask.w *= MakeGradient(0.0, 1.0, pupilPosition, 0.5, 2.0);
}