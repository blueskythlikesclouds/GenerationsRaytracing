#pragma once

#include "ComputeLuminancePass.h"
#include "RaytracingPass.h"
#include "ToneMapPass.h"

struct Scene;

struct Renderer
{
    nvrhi::BufferHandle constantBuffer;
    nvrhi::CommandListHandle commandList;
    nvrhi::DrawArguments quadDrawArgs;

    RaytracingPass raytracingPass;
    ComputeLuminancePass computeLuminancePass;
    std::vector<ToneMapPass> toneMapPasses;

    uint32_t sampleCount = 0;

    Renderer(const App& app);

    void execute(const App& app, Scene& scene);
};
