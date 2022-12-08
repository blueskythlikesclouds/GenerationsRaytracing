#include "App.h"

App::App(const std::string& directoryPath, int width, int height)
    : device(), window(device, width, height), shaderLibrary(directoryPath), renderer(device, window, shaderLibrary)
{
}

void App::run(Scene& scene)
{
    auto prevTime = std::chrono::system_clock::now();

    while (!window.shouldClose)
    {
        const auto currentTime = std::chrono::system_clock::now();
        const float deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - prevTime).count();

        window.processMessages();
        camera.update(window.input, deltaTime);
        renderer.update(*this, deltaTime, scene);
        device.nvrhi->waitForIdle();
        window.present();

        prevTime = currentTime;
    }
}
