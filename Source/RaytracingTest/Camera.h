#pragma once

struct App;

struct Camera
{
    Eigen::Vector3f position;
    Eigen::Quaternionf rotation;
    float fieldOfView = 90.0f;
    float aspectRatio = 1.0f;

    bool dirty = false;

    Camera() :
        position(Eigen::Vector3f::Zero()),
        rotation(Eigen::Quaternionf::Identity())
    {
        
    }

    void update(const App& app);
};
