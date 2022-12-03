#pragma once

#include "Camera.h"
#include "Device.h"
#include "Renderer.h"
#include "Window.h"

struct App
{
    Device device;
    Window window;
    Renderer renderer;
    Camera camera;

    App(int width, int height);

    void run(Scene& scene);
};
