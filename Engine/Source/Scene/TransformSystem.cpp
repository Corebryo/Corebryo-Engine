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
#include <cstddef>

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

/* Add cache entry for a transform. */
void TransformSystem::AddTransform(std::uint32_t id)
{
    /* Grow sparse index array when needed. */
    if (id >= indexByEntity.size())
    {
        const std::size_t newSize = static_cast<std::size_t>(id) + 1;
        indexByEntity.resize(newSize, kInvalidIndex);
    }

    /* Check for an existing cache entry. */
    const std::uint32_t existingIndex = indexByEntity[id];
    if (existingIndex != kInvalidIndex)
    {
        /* Mark existing cache as dirty. */
        dirty[existingIndex] = true;
        return;
    }

    /* Append a new packed cache slot. */
    const std::uint32_t newIndex = static_cast<std::uint32_t>(modelCache.size());
    entityIds.push_back(id);
    modelCache.push_back(identityCache);
    dirty.push_back(1);
    indexByEntity[id] = newIndex;
}

/* Remove cache entry for a transform. */
void TransformSystem::RemoveTransform(std::uint32_t id)
{
    /* Resolve packed index for entity id. */
    const std::uint32_t index = GetIndex(id);

    if (index == kInvalidIndex)
    {
        return;
    }

    /* Swap with the last element to keep storage packed. */
    const std::uint32_t lastIndex =
        static_cast<std::uint32_t>(modelCache.size() - 1);

    if (index != lastIndex)
    {
        /* Move the last packed entry into the removed slot. */
        entityIds[index] = entityIds[lastIndex];
        modelCache[index] = modelCache[lastIndex];
        dirty[index] = dirty[lastIndex];
        indexByEntity[entityIds[index]] = index;
    }

    /* Remove the last packed entry. */
    entityIds.pop_back();
    modelCache.pop_back();
    dirty.pop_back();
    indexByEntity[id] = kInvalidIndex;
}

/* Query cache presence for entity id. */
bool TransformSystem::HasTransform(std::uint32_t id) const
{
    /* Check if entity has a valid packed index. */
    return GetIndex(id) != kInvalidIndex;
}

/* Get cached model matrix, rebuilding when dirty. */
const Mat4& TransformSystem::GetModelMatrix(
    std::uint32_t id,
    const TransformComponent& transform) const
{
    /* Resolve packed index for entity id. */
    const std::uint32_t index = GetIndex(id);

    if (index == kInvalidIndex)
    {
        return identityCache;
    }

    if (dirty[index])
    {
        /* Rebuild the cached model matrix. */
        modelCache[index] = BuildModelMatrix(transform);
        dirty[index] = 0;
    }

    return modelCache[index];
}

/* Explicitly mark transform dirty. */
void TransformSystem::MarkDirty(std::uint32_t id)
{
    /* Resolve packed index for entity id. */
    const std::uint32_t index = GetIndex(id);

    if (index == kInvalidIndex)
    {
        return;
    }

    dirty[index] = 1;
}

/* Reset all transforms. */
void TransformSystem::Clear()
{
    /* Clear packed storage and sparse indices. */
    entityIds.clear();
    modelCache.clear();
    dirty.clear();
    indexByEntity.clear();
}

/* Recompute cached model matrix. */
Mat4 TransformSystem::BuildModelMatrix(const TransformComponent& transform) const
{
    /* Compose the model matrix using TRS order. */
    return
        BuildTranslation(transform.Position) *
        BuildRotationZ(transform.Rotation.z) *
        BuildRotationY(transform.Rotation.y) *
        BuildRotationX(transform.Rotation.x) *
        BuildScale(transform.Scale);
}

/* Resolve packed index for an entity id. */
std::uint32_t TransformSystem::GetIndex(std::uint32_t id) const
{
    /* Validate the sparse lookup range. */
    if (id >= indexByEntity.size())
    {
        return kInvalidIndex;
    }

    /* Return the packed index. */
    return indexByEntity[id];
}
