#pragma once

#include "ComputeLuminancePass.h"
#include "DLSSPass.h"
#include "RaytracingPass.h"
#include "ToneMapPass.h"

struct Scene;

struct Renderer
{
    nvrhi::BufferHandle constantBuffer;
    nvrhi::CommandListHandle commandList;
    nvrhi::DrawArguments quadDrawArgs;

    RaytracingPass raytracingPass;
    DLSSPass dlssPass;
    ComputeLuminancePass computeLuminancePass;
    std::vector<ToneMapPass> toneMapPasses;

    Renderer(const App& app, const std::string& directoryPath);

    void execute(const App& app, Scene& scene);
};
