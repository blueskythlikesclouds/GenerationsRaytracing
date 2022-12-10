#include "App.h"

App::App(int width, int height, const std::string& directoryPath) :
    width(width),
    height(height),
    device(),
    window(*this),
    input(),
    shaderLibrary(*this, directoryPath),
    renderer(*this, directoryPath),
    camera()
{
}

void App::run(Scene& scene)
{
    auto prevTime = std::chrono::system_clock::now();

    while (!window.shouldClose)
    {
        const auto currentTime = std::chrono::system_clock::now();
        deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - prevTime).count();

        input.recordHistory();
        window.processMessages();
        camera.update(*this);
        renderer.execute(*this, scene);
        device.nvrhi->waitForIdle();
        device.nvrhi->runGarbageCollection();
        window.present();

        prevTime = currentTime;
    }
}
