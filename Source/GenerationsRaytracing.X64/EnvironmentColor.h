#pragma once

struct GlobalsPS;

struct EnvironmentColor
{
    static bool get(const GlobalsPS& globalsPS, float* skyColor, float* groundColor);
};
