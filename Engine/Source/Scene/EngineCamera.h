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

#pragma once

#include "../Math/MathTypes.h"

/* Manages camera orientation, movement, and view transforms. */
class Camera
{
public:
    /* Initialize camera defaults. */
    Camera();

    /* Cleanup camera state. */
    ~Camera();

    /* Update camera based on input and elapsed time. */
    void Update(float DeltaTime);

    /* Compute MVP matrix for rendering. */
    Mat4 GetMVPMatrix(float AspectRatio, const Mat4& Model) const;

    /* Set world-space camera position. */
    void SetPosition(const Vec3& Position);

    /* Get world-space camera position. */
    Vec3 GetPosition() const;

    /* Set camera rotation in yaw and pitch. */
    void SetRotation(float Yaw, float Pitch);

    /* Direction vectors. */
    Vec3 GetFront() const { return Front; }
    Vec3 GetUp() const { return Up; }
    Vec3 GetRight() const { return Right; }

    /* Field of view zoom factor. */
    float GetZoom() const { return Zoom; }

private:
    /* Camera transform vectors. */
    Vec3 Position;
    Vec3 Front;
    Vec3 Up;
    Vec3 Right;
    Vec3 WorldUp;

    /* Euler rotation angles. */
    float Yaw;
    float Pitch;

    /* Camera tuning parameters. */
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    /* Recalculate direction vectors from rotation. */
    void UpdateCameraVectors();
};
