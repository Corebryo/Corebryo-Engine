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

#include "TransformSystem.h"

#include <cmath>

namespace
{
    /* Matrix storage is assumed to be column-major with translation at [12..14]. */
    constexpr int kTx = 12;
    constexpr int kTy = 13;
    constexpr int kTz = 14;

    /* Build translation matrix from world position. */
    Mat4 BuildTranslation(const Vec3& position)
    {
        Mat4 m = Mat4::Identity();

        m.m[kTx] = position.x;
        m.m[kTy] = position.y;
        m.m[kTz] = position.z;

        return m;
    }

    /* Build scale matrix from per-axis scale. */
    Mat4 BuildScale(const Vec3& scale)
    {
        Mat4 m = Mat4::Identity();

        m.m[0] = scale.x;
        m.m[5] = scale.y;
        m.m[10] = scale.z;

        return m;
    }

    /* Build rotation matrix around X axis. */
    Mat4 BuildRotationX(float angle)
    {
        Mat4 m = Mat4::Identity();

        const float c = std::cos(angle);
        const float s = std::sin(angle);

        m.m[5] = c;
        m.m[6] = s;
        m.m[9] = -s;
        m.m[10] = c;

        return m;
    }

    /* Build rotation matrix around Y axis. */
    Mat4 BuildRotationY(float angle)
    {
        Mat4 m = Mat4::Identity();

        const float c = std::cos(angle);
        const float s = std::sin(angle);

        m.m[0] = c;
        m.m[2] = -s;
        m.m[8] = s;
        m.m[10] = c;

        return m;
    }

    /* Build rotation matrix around Z axis. */
    Mat4 BuildRotationZ(float angle)
    {
        Mat4 m = Mat4::Identity();

        const float c = std::cos(angle);
        const float s = std::sin(angle);

        m.m[0] = c;
        m.m[1] = s;
        m.m[4] = -s;
        m.m[5] = c;

        return m;
    }
}

/* Initialize default caches. */
TransformSystem::TransformSystem()
    : identityCache(Mat4::Identity())
{
}

/* Containers clean themselves up. */
TransformSystem::~TransformSystem()
{
}

/* Access transform for modification. */
TransformComponent* TransformSystem::GetComponent(std::uint32_t id)
{
    if (!IsValidId(id) || !present[id])
    {
        return nullptr;
    }

    dirty[id] = true;
    return &components[id];
}

/* Access transform without marking dirty. */
const TransformComponent* TransformSystem::GetComponent(std::uint32_t id) const
{
    if (!IsValidId(id) || !present[id])
    {
        return nullptr;
    }

    return &components[id];
}

/* Add transform for entity id. */
TransformComponent& TransformSystem::AddComponent(std::uint32_t id)
{
    EnsureSize(id);

    present[id] = true;
    dirty[id] = true;
    modelCache[id] = identityCache;

    return components[id];
}

/* Remove transform for entity id. */
void TransformSystem::RemoveComponent(std::uint32_t id)
{
    if (!IsValidId(id))
    {
        return;
    }

    present[id] = false;
    dirty[id] = false;
    components[id] = TransformComponent();
    modelCache[id] = identityCache;
}

/* Query presence for entity id. */
bool TransformSystem::HasComponent(std::uint32_t id) const
{
    if (!IsValidId(id))
    {
        return false;
    }

    return present[id] != 0;
}

/* Get cached model matrix, rebuilding when dirty. */
const Mat4& TransformSystem::GetModelMatrix(std::uint32_t id) const
{
    if (!IsValidId(id) || !present[id])
    {
        return identityCache;
    }

    if (dirty[id])
    {
        modelCache[id] = BuildModelMatrix(components[id]);
        dirty[id] = false;
    }

    return modelCache[id];
}

/* Explicitly mark transform dirty. */
void TransformSystem::MarkDirty(std::uint32_t id)
{
    if (!IsValidId(id) || !present[id])
    {
        return;
    }

    dirty[id] = true;
}

/* Ensure storage covers entity id. */
void TransformSystem::EnsureSize(std::uint32_t id)
{
    if (IsValidId(id))
    {
        return;
    }

    const std::size_t newSize = static_cast<std::size_t>(id) + 1;
    components.resize(newSize);
    modelCache.resize(newSize, identityCache);
    present.resize(newSize, 0);
    dirty.resize(newSize, 0);
}

/* Reset all transforms. */
void TransformSystem::Clear()
{
    components.clear();
    modelCache.clear();
    present.clear();
    dirty.clear();
}

/* Recompute cached model matrix. */
Mat4 TransformSystem::BuildModelMatrix(const TransformComponent& transform) const
{
    return
        BuildTranslation(transform.Position) *
        BuildRotationZ(transform.Rotation.z) *
        BuildRotationY(transform.Rotation.y) *
        BuildRotationX(transform.Rotation.x) *
        BuildScale(transform.Scale);
}

/* Validate id against storage. */
bool TransformSystem::IsValidId(std::uint32_t id) const
{
    return static_cast<std::size_t>(id) < components.size();
}
