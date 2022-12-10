#include "App.h"
#include "Path.h"
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

    const std::string directoryPath = getDirectoryPath(argv[0]);

    App app(INT_MAX, INT_MAX, directoryPath.empty() ? "." : directoryPath);
    app.run(scene);

    return 0;
}
