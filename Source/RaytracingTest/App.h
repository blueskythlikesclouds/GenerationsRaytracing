#pragma once

#include "Camera.h"
#include "Device.h"
#include "Renderer.h"
#include "ShaderLibrary.h"
#include "Window.h"

struct App
{
    Device device;
    Window window;
    ShaderLibrary shaderLibrary;
    Renderer renderer;
    Camera camera;

    App(const std::string& directoryPath, int width, int height);

    void run(Scene& scene);
};
