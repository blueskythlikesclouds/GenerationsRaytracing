#pragma once

#include "Camera.h"
#include "Device.h"
#include "Input.h"
#include "Renderer.h"
#include "ShaderLibrary.h"
#include "Window.h"

struct App
{
    int width = 0;
    int height = 0;
    Device device;
    Window window;
    Input input;
    ShaderLibrary shaderLibrary;
    Renderer renderer;
    CameraController camera;
    float deltaTime = 0.0f;

    App(int width, int height, const std::string& directoryPath);

    void run(Scene& scene);
};
