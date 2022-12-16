#include "Camera.h"
#include "App.h"
#include "Math.h"
#include "ShaderDefinitions.hlsli"

void CameraController::update(const App& app)
{
    previous = *static_cast<Camera*>(this);
    dirty = false;

    if (app.input.key['Q'] ^ app.input.key['E'])
    {
        const Eigen::AngleAxisf z((app.input.key['E'] ? -2.0f : 2.0f) * app.deltaTime, rotation * Eigen::Vector3f::UnitZ());
        rotation = (z * rotation).normalized();

        dirty = true;
    }

    if (app.input.rightMouse && (app.input.prevCursorX != app.input.cursorX || app.input.prevCursorY != app.input.cursorY))
    {
        const float pitch = (float)(app.input.prevCursorY - app.input.cursorY) * 0.005f;
        const float yaw = (float)(app.input.prevCursorX - app.input.cursorX) * 0.005f;

        const Eigen::AngleAxisf x(pitch, rotation * Eigen::Vector3f::UnitX());
        const Eigen::AngleAxisf y(yaw, Eigen::Vector3f::UnitY());

        rotation = (x * rotation).normalized();
        rotation = (y * rotation).normalized();

        dirty = true;
    }

    const float speed = app.input.key[VK_SHIFT] ? 120.0f : 30.0f;

    if (app.input.key['W'] ^ app.input.key['S'])
    {
        position += (rotation * Eigen::Vector3f::UnitZ()).normalized() * (app.input.key['W'] ? -speed : speed) * app.deltaTime;
        dirty = true;
    }

    if (app.input.key['A'] ^ app.input.key['D'])
    {
        position += (rotation * Eigen::Vector3f::UnitX()).normalized() * (app.input.key['A'] ? -speed : speed) * app.deltaTime;
        dirty = true;
    }

    if (app.input.key[VK_CONTROL] ^ app.input.key[VK_SPACE])
    {
        position += Eigen::Vector3f::UnitY() * (app.input.key[VK_CONTROL] ? -speed : speed) * app.deltaTime;
        dirty = true;
    }

    if (app.input.key[VK_ADD] ^ app.input.key[VK_SUBTRACT])
    {
        fieldOfView = fmodf(fieldOfView + (app.input.key[VK_ADD] ? 1.0f : -1.0f) / 180.0f * (float)M_PI, (float)M_PI);
        dirty = true;
    }

    RECT clientRect;
    GetClientRect(app.window.handle, &clientRect);

    const float newAspectRatio =
        (float)(clientRect.right - clientRect.left) /
        (float)(clientRect.bottom - clientRect.top);

    if (aspectRatio != newAspectRatio)
    {
        aspectRatio = newAspectRatio;
        dirty = true;
    }

    if (dirty)
    {
        view = (Eigen::Translation3f(position) * rotation).inverse().matrix();
        projection = Eigen::CreatePerspective(fieldOfView, aspectRatio, 0.001f, Z_MAX);
        currentAccum = 0;
    }

    pixelJitter = haltonJitter(++currentFrame, 64);
    ++currentAccum;
}
