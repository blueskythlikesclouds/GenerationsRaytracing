#include "App.h"

App::App(int width, int height)
    : device(), window(device, width, height), renderer(device, window)
{
}

void App::run(Scene& scene)
{
    while (!window.shouldClose)
    {
        window.processMessages();
        camera.update(window.input);
        renderer.update(*this, scene);
        device.nvrhi->waitForIdle();
        window.present();
    }
}
