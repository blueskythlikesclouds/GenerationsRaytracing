#include "Camera.h"

#include "Input.h"

void Camera::update(const Input& input)
{
    const float deltaTime = 1.0f / 60.0f;

    if (input.key['Q'] ^ input.key['E'])
    {
        const Eigen::AngleAxisf z((input.key['E'] ? -2.0f : 2.0f) * deltaTime, rotation * Eigen::Vector3f::UnitZ());
        rotation = (z * rotation).normalized();
    }

    if (input.rightMouse && (input.prevCursorX != input.cursorX || input.prevCursorY != input.cursorY))
    {
        const float pitch = (float)(input.cursorY - input.prevCursorY) * 0.25f * -deltaTime;
        const float yaw = (float)(input.cursorX - input.prevCursorX) * 0.25f * -deltaTime;

        const Eigen::AngleAxisf x(pitch, rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        rotation = (x * rotation).normalized();
        rotation = (y * rotation).normalized();
    }

    const float speed = input.key[VK_LSHIFT] ? 120.0f : 30.0f;

    if (input.key['W'] ^ input.key['S'])
    {
        position += (rotation * Eigen::Vector3f::UnitZ()).normalized() * (input.key['W'] ? -speed : speed) * deltaTime;
    }

    if (input.key['A'] ^ input.key['D'])
    {
        position += (rotation * Eigen::Vector3f::UnitX()).normalized() * (input.key['A'] ? -speed : speed) * deltaTime;
    }

    if (input.key[VK_LCONTROL] ^ input.key[VK_SPACE])
    {
        position += Eigen::Vector3f::UnitY() * (input.key[VK_LCONTROL] ? -speed : speed) * deltaTime;
    }
}
