#include "Renderer.h"

#include "App.h"
#include "Scene.h"

struct ConstantBuffer
{
    Eigen::Vector3f position;
    float tanFovy;
    Eigen::Matrix4f rotation;

    float aspectRatio;
    uint32_t sampleCount;
    float skyIntensityScale;
    float deltaTime;

    Eigen::Vector4f lightDirection;
    Eigen::Vector4f lightColor;

    Eigen::Vector4f lightScatteringColor;
    Eigen::Vector4f rayMieRay2Mie2;
    Eigen::Vector4f gAndFogDensity;
    Eigen::Vector4f farNearScale;

    Eigen::Vector4f eyeLightDiffuse;
    Eigen::Vector4f eyeLightSpecular;
    Eigen::Vector4f eyeLightRange;
    Eigen::Vector4f eyeLightAttribute;

    float middleGray;
    float lumMin;
    float lumMax;
    uint32_t lightCount;
};

Renderer::Renderer(const App& app) :
    constantBuffer(app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(ConstantBuffer))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1))),

    commandList(app.device.nvrhi->createCommandList()),

    raytracingPass(app),
    computeLuminancePass(app)
{
    quadDrawArgs.setVertexCount(6);

    for (auto& framebuffer : app.window.nvrhi.swapChainFramebuffers)
        toneMapPasses.emplace_back(app, framebuffer);
}

void Renderer::execute(const App& app, Scene& scene)
{
    if (app.camera.dirty)
        sampleCount = 0;

    ConstantBuffer bufferData{};

    bufferData.position = app.camera.position;
    bufferData.tanFovy = tan(app.camera.fieldOfView / 360.0f * (float)M_PI);
    bufferData.rotation.block(0, 0, 3, 3) = app.camera.rotation.toRotationMatrix();

    bufferData.aspectRatio = app.camera.aspectRatio;
    bufferData.sampleCount = ++sampleCount;
    bufferData.skyIntensityScale = scene.cpu.effect.defaults.skyIntensityScale;
    bufferData.deltaTime = app.deltaTime;

    bufferData.lightDirection.head<3>() = scene.cpu.globalLight.position;
    bufferData.lightColor.head<3>() = scene.cpu.globalLight.color;

    bufferData.lightScatteringColor = scene.cpu.effect.lightScattering.color;
    bufferData.rayMieRay2Mie2 = scene.cpu.effect.lightScattering.gpu.rayMieRay2Mie2;
    bufferData.gAndFogDensity = scene.cpu.effect.lightScattering.gpu.gAndFogDensity;
    bufferData.farNearScale = scene.cpu.effect.lightScattering.gpu.farNearScale;

    bufferData.eyeLightDiffuse = scene.cpu.effect.eyeLight.diffuse;
    bufferData.eyeLightSpecular = scene.cpu.effect.eyeLight.specular;
    bufferData.eyeLightRange = scene.cpu.effect.eyeLight.gpu.range;
    bufferData.eyeLightAttribute = scene.cpu.effect.eyeLight.gpu.attribute;

    bufferData.middleGray = scene.cpu.effect.hdr.middleGray;
    bufferData.lumMin = scene.cpu.effect.hdr.lumMin;
    bufferData.lumMax = scene.cpu.effect.hdr.lumMax;

    bufferData.lightCount = (uint32_t)scene.cpu.localLights.size();

    commandList->open();
    commandList->writeBuffer(constantBuffer, &bufferData, sizeof(ConstantBuffer));

    raytracingPass.execute(app, scene);
    computeLuminancePass.execute(app);
    toneMapPasses[app.window.getBackBufferIndex()].execute(app);

    commandList->close();

    app.device.nvrhi->executeCommandList(commandList);
}
