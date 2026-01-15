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

#include "../../Math/MathTypes.h"
#include "../Collision/AABB.h"

/* Position, rotation, and scale component. */
struct TransformComponent
{
    /* World-space position. */
    Vec3 Position = Vec3(0.0f, 0.0f, 0.0f);

    /* Euler rotation in radians or degrees. */
    Vec3 Rotation = Vec3(0.0f, 0.0f, 0.0f);

    /* Non-uniform world-space scale. */
    Vec3 Scale = Vec3(1.0f, 1.0f, 1.0f);

    /* Compute axis-aligned bounding box for a unit cube. */
    AABB GetUnitCubeAABB() const
    {
        /* Compute half extents from scale. */
        const Vec3 HalfExtents(
            Scale.x * 0.5f,
            Scale.y * 0.5f,
            Scale.z * 0.5f);

        /* Build AABB in world space. */
        return AABB{ Position - HalfExtents, Position + HalfExtents };
    }
};
