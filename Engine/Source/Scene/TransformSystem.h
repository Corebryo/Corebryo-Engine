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

/* Manages transforms and cached model matrices. */
class TransformSystem
{
public:
    TransformSystem();
    ~TransformSystem();

    /* Access transform for modification. */
    TransformComponent* GetComponent(std::uint32_t id);

    /* Access transform without marking dirty. */
    const TransformComponent* GetComponent(std::uint32_t id) const;

    /* Add transform for entity id. */
    TransformComponent& AddComponent(std::uint32_t id);

    /* Remove transform for entity id. */
    void RemoveComponent(std::uint32_t id);

    /* Query presence for entity id. */
    bool HasComponent(std::uint32_t id) const;

    /* Get cached model matrix, rebuilding when dirty. */
    const Mat4& GetModelMatrix(std::uint32_t id) const;

    /* Explicitly mark transform dirty. */
    void MarkDirty(std::uint32_t id);

    /* Ensure storage covers entity id. */
    void EnsureSize(std::uint32_t id);

    /* Reset all transforms. */
    void Clear();

private:
    /* Recompute cached model matrix. */
    Mat4 BuildModelMatrix(const TransformComponent& transform) const;

    /* Validate id against storage. */
    bool IsValidId(std::uint32_t id) const;

private:
    std::vector<TransformComponent> components;
    mutable std::vector<Mat4> modelCache;
    std::vector<std::uint8_t> present;
    mutable std::vector<std::uint8_t> dirty;
    Mat4 identityCache;
};
