#pragma once

struct Input;

struct Camera
{
    Eigen::Vector3f position;
    Eigen::Quaternionf rotation;
    float fieldOfView = 90.0f;
    bool dirty = false;

    Camera() :
        position(Eigen::Vector3f::Zero()),
        rotation(Eigen::Quaternionf::Identity())
    {
        
    }

    void update(const Input& input, float deltaTime);
};
