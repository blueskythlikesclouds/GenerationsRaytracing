#pragma once

struct App;

struct Camera
{
    Eigen::Vector3f position;
    Eigen::Quaternionf rotation;
    Eigen::Matrix4f view;
    Eigen::Matrix4f projection;
    float fieldOfView = (float) M_PI_2;
    float aspectRatio = 1.0f;
    int currentFrame = -1;
    Eigen::Vector2f pixelJitter = Eigen::Vector2f::Zero();

    Camera() :
        position(Eigen::Vector3f::Zero()),
        rotation(Eigen::Quaternionf::Identity())
    {

    }
};

struct CameraController : Camera
{
    Camera previous;
    bool dirty = false;

    void update(const App& app);
};
