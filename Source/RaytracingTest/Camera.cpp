#include "Camera.h"

#include "Input.h"

void Camera::update(const Input& input, const float deltaTime)
{
    dirty = false;

    if (input.key['Q'] ^ input.key['E'])
    {
        const Eigen::AngleAxisf z((input.key['E'] ? -2.0f : 2.0f) * deltaTime, rotation * Eigen::Vector3f::UnitZ());
        rotation = (z * rotation).normalized();

        dirty = true;
    }

    if (input.rightMouse && (input.prevCursorX != input.cursorX || input.prevCursorY != input.cursorY))
    {
        const float pitch = (float)(input.prevCursorY - input.cursorY) * 0.005f;
        const float yaw = (float)(input.prevCursorX - input.cursorX) * 0.005f;

        const Eigen::AngleAxisf x(pitch, rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        rotation = (x * rotation).normalized();
        rotation = (y * rotation).normalized();

        dirty = true;
    }

    const float speed = input.key[VK_SHIFT] ? 120.0f : 30.0f;

    if (input.key['W'] ^ input.key['S'])
    {
        position += (rotation * Eigen::Vector3f::UnitZ()).normalized() * (input.key['W'] ? -speed : speed) * deltaTime;
        dirty = true;
    }

    if (input.key['A'] ^ input.key['D'])
    {
        position += (rotation * Eigen::Vector3f::UnitX()).normalized() * (input.key['A'] ? -speed : speed) * deltaTime;
        dirty = true;
    }

    if (input.key[VK_CONTROL] ^ input.key[VK_SPACE])
    {
        position += Eigen::Vector3f::UnitY() * (input.key[VK_CONTROL] ? -speed : speed) * deltaTime;
        dirty = true;
    }

    if (input.key[VK_ADD] ^ input.key[VK_SUBTRACT])
    {
        fieldOfView = fmodf(fieldOfView + (input.key[VK_ADD] ? 1.0f : -1.0f), 180.0f);
        dirty = true;
    }
}
