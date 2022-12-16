#include "Renderer.h"

#include "App.h"
#include "Math.h"
#include "Scene.h"

struct ConstantBuffer
{
    Eigen::Vector3f position;
    float tanFovy;
    Eigen::Matrix4f rotation;

    Eigen::Matrix4f view;
    Eigen::Matrix4f projection;

    Eigen::Matrix4f previousView;
    Eigen::Matrix4f previousProjection;

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

    Eigen::Vector2f pixelJitter;

    float currentAccum;
};

Renderer::Renderer(const App& app, const std::string& directoryPath) :
    constantBuffer(app.device.nvrhi->createBuffer(nvrhi::BufferDesc()
        .setByteSize(sizeof(ConstantBuffer))
        .setIsConstantBuffer(true)
        .setIsVolatile(true)
        .setMaxVersions(1))),

    commandList(app.device.nvrhi->createCommandList()),

    raytracingPass(app),
    dlssPass(app, directoryPath),
    accumulatePass(app),
    computeLuminancePass(app)
{
    quadDrawArgs.setVertexCount(6);

    for (auto& framebuffer : app.window.nvrhi.swapChainFramebuffers)
        toneMapPasses.emplace_back(app, framebuffer);
}

void Renderer::execute(const App& app, Scene& scene)
{
    ConstantBuffer bufferData{};

    bufferData.position = app.camera.position;
    bufferData.tanFovy = tan(app.camera.fieldOfView / 2.0f);
    bufferData.rotation.block(0, 0, 3, 3) = app.camera.rotation.toRotationMatrix();

    bufferData.view = app.camera.view;
    bufferData.projection = app.camera.projection;

    bufferData.previousView = app.camera.previous.view;
    bufferData.previousProjection = app.camera.previous.projection;

    bufferData.aspectRatio = app.camera.aspectRatio;
    bufferData.sampleCount = app.camera.currentFrame;
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

    bufferData.pixelJitter = app.camera.pixelJitter;

    bufferData.currentAccum = 1.0f / (float)app.camera.currentAccum;

    commandList->open();
    commandList->writeBuffer(constantBuffer, &bufferData, sizeof(ConstantBuffer));

    raytracingPass.execute(app, scene);
    dlssPass.execute(app);
    accumulatePass.execute(app);
    computeLuminancePass.execute(app);
    toneMapPasses[app.window.getBackBufferIndex()].execute(app);

    commandList->close();

    app.device.nvrhi->executeCommandList(commandList);
}
