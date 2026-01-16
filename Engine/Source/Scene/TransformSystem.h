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

#include "Components/TransformComponent.h"
#include "Math/MathTypes.h"

#include <cstdint>
#include <vector>

/* Manages cached model matrices for transforms. */
class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();
    TransformSystem(const TransformSystem& other) = delete;
    TransformSystem& operator=(const TransformSystem& other) = delete;
    TransformSystem(TransformSystem&& other) noexcept = default;
    TransformSystem& operator=(TransformSystem&& other) noexcept = default;

    /* Add cache entry for a transform. */
    void AddTransform(std::uint32_t id);

    /* Remove cache entry for a transform. */
    void RemoveTransform(std::uint32_t id);

    /* Query cache presence for entity id. */
    bool HasTransform(std::uint32_t id) const;

    /* Get cached model matrix, rebuilding when dirty. */
    const Mat4& GetModelMatrix(
        std::uint32_t id,
        const TransformComponent& transform) const;

    /* Explicitly mark transform dirty. */
    void MarkDirty(std::uint32_t id);

    /* Reset all transforms. */
    void Clear();

private:
    /* Recompute cached model matrix. */
    Mat4 BuildModelMatrix(const TransformComponent& transform) const;

    /* Resolve packed index for entity id. */
    std::uint32_t GetIndex(std::uint32_t id) const;

private:
    /* Invalid index sentinel for sparse mapping. */
    static constexpr std::uint32_t kInvalidIndex = 0xFFFFFFFFu;

    /* Entity ids for cached transforms. */
    std::vector<std::uint32_t> entityIds;

    /* Sparse lookup from entity id to packed index. */
    std::vector<std::uint32_t> indexByEntity;

    /* Cached model matrices per packed component. */
    mutable std::vector<Mat4> modelCache;

    /* Dirty flags per packed component. */
    mutable std::vector<std::uint8_t> dirty;

    /* Identity matrix cache for invalid lookups. */
    Mat4 identityCache;
};
