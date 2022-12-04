#include "App.h"
#include "Scene.h"

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: RaytracingTest [stage]");
        return 0;
    }

    Scene scene;
    scene.loadCpuResources(argv[1]);

    App app(1280, 720);
    app.run(scene);

    return 0;
}
