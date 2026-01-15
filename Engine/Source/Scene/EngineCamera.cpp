/*
 * Corebryo
 * Copyright (c) 2026 Jonathan Den Haerynck
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "EngineCamera.h"

#include "../Input/InputState.h"

#include <cmath>

/* Math constants. */
namespace
{
    constexpr float Pi = 3.1415926535f;
}

/* Initialize camera state. */
Camera::Camera()
    : Position(0.0f, 50.0f, -50.0f)
    , Front(0.0f, 0.0f, -1.0f)
    , Up(0.0f, 1.0f, 0.0f)
    , Right(1.0f, 0.0f, 0.0f)
    , WorldUp(0.0f, 1.0f, 0.0f)
    , Yaw(-90.0f)
    , Pitch(0.0f)
    , MovementSpeed(15.0f)
    , MouseSensitivity(0.1f)
    , Zoom(45.0f)
{
    /* Build initial orientation vectors. */
    UpdateCameraVectors();
}

/* Destroy camera. */
Camera::~Camera()
{
}

/* Update camera movement and rotation. */
void Camera::Update(float DeltaTime)
{
    const InputFrame& input = GetFrameInput();

    /* Compute movement speed. */
    const float speed =
        MovementSpeed * DeltaTime *
        (GetActionState(InputAction::FastMove) ? 2.0f : 1.0f);

    /* Process movement input. */
    if (GetActionState(InputAction::MoveForward))
        Position = Position + Front * speed;

    if (GetActionState(InputAction::MoveBackward))
        Position = Position - Front * speed;

    if (GetActionState(InputAction::MoveLeft))
        Position = Position - Right * speed;

    if (GetActionState(InputAction::MoveRight))
        Position = Position + Right * speed;

    /* Apply rotation input. */
    SetRotation(input.Yaw, input.Pitch);
}

/* Compute model-view-projection matrix. */
Mat4 Camera::GetMVPMatrix(float AspectRatio, const Mat4& Model) const
{
    /* Build view matrix. */
    const Mat4 View =
        Mat4::LookAt(Position, Position + Front, Up);

    /* Build projection matrix. */
    const Mat4 Projection =
        Mat4::Perspective(Zoom * Pi / 180.0f, AspectRatio, 0.1f, 100.0f);

    /* Return combined transform. */
    return Projection * View * Model;
}

/* Set world-space position. */
void Camera::SetPosition(const Vec3& Position)
{
    this->Position = Position;
}

/* Get world-space position. */
Vec3 Camera::GetPosition() const
{
    return Position;
}

/* Set yaw and pitch rotation. */
void Camera::SetRotation(float Yaw, float Pitch)
{
    this->Yaw = Yaw;
    this->Pitch = Pitch;

    /* Clamp pitch to prevent flipping. */
    if (this->Pitch > 89.0f)
        this->Pitch = 89.0f;

    if (this->Pitch < -89.0f)
        this->Pitch = -89.0f;

    UpdateCameraVectors();
}

/* Recalculate orientation vectors. */
void Camera::UpdateCameraVectors()
{
    /* Compute forward vector. */
    Vec3 front;
    front.x = std::cos(Yaw * Pi / 180.0f) * std::cos(Pitch * Pi / 180.0f);
    front.y = std::sin(Pitch * Pi / 180.0f);
    front.z = std::sin(Yaw * Pi / 180.0f) * std::cos(Pitch * Pi / 180.0f);

    Front = front.Normalized();

    /* Rebuild orthogonal basis. */
    Right = Vec3::Cross(Front, WorldUp).Normalized();
    Up = Vec3::Cross(Right, Front).Normalized();
}
