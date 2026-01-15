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

#include "Scene.h"

#include <cmath>
#include <cstdint>

 /* Local math helpers for building transform matrices. */
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

    /* Compose a model matrix from TRS, using the engine's multiplication convention. */
    Mat4 BuildModelMatrix(const TransformComponent& transform)
    {
        /* Order is T * Rz * Ry * Rx * S, matching the original behavior. */
        return
            BuildTranslation(transform.Position) *
            BuildRotationZ(transform.Rotation.z) *
            BuildRotationY(transform.Rotation.y) *
            BuildRotationX(transform.Rotation.x) *
            BuildScale(transform.Scale);
    }

    /* Validate that an entity id is inside the scene arrays. */
    bool IsValidId(std::uint32_t id, std::size_t count)
    {
        return static_cast<std::size_t>(id) < count;
    }
}

/* Initialize empty scene state. */
Scene::Scene()
{
    /* Vectors default-initialize, nothing required here. */
}

/* Scene cleanup handled by containers. */
Scene::~Scene()
{
    /* std::vector releases its memory automatically. */
}

/* Create a new entity and mark it alive. */
Entity Scene::CreateEntity()
{
    /* This uses a simple monotonic id allocation. */
    const std::uint32_t id = static_cast<std::uint32_t>(alive.size());

    EnsureSize(id);
    alive[id] = true;

    return Entity(id, this);
}

/* Destroy entity and clear all component flags. */
void Scene::DestroyEntity(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ignore invalid ids to keep the API forgiving. */
    if (!IsValidId(id, alive.size()))
    {
        return;
    }

    /* Mark entity dead first, so it will never be considered renderable. */
    alive[id] = false;

    /* Drop component presence bits. */
    hasTransform[id] = false;
    hasMesh[id] = false;
    hasMaterial[id] = false;
}

/* Retrieve transform component if present. */
TransformComponent* Scene::GetTransform(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Validate index and presence bit. */
    if (id >= transforms.size() || !hasTransform[id])
    {
        return nullptr;
    }

    return &transforms[id];
}

/* Retrieve mesh component if present. */
MeshComponent* Scene::GetMesh(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Validate index and presence bit. */
    if (id >= meshes.size() || !hasMesh[id])
    {
        return nullptr;
    }

    return &meshes[id];
}

/* Retrieve material component if present. */
MaterialComponent* Scene::GetMaterial(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Validate index and presence bit. */
    if (id >= materials.size() || !hasMaterial[id])
    {
        return nullptr;
    }

    return &materials[id];
}

/* Attach transform component to entity. */
TransformComponent& Scene::AddTransform(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Mark component present, data lives in a dense vector. */
    hasTransform[id] = true;

    return transforms[id];
}

/* Attach mesh component to entity. */
MeshComponent& Scene::AddMesh(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Mark component present, data lives in a dense vector. */
    hasMesh[id] = true;

    return meshes[id];
}

/* Attach material component to entity. */
MaterialComponent& Scene::AddMaterial(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Mark component present, data lives in a dense vector. */
    hasMaterial[id] = true;

    return materials[id];
}

/* Build renderable items from active scene entities. */
void Scene::BuildRenderList(std::vector<RenderItem>& outItems) const
{
    /* Caller gets a clean list every time. */
    outItems.clear();

    /* Avoid repeated reallocations when many entities are alive. */
    outItems.reserve(alive.size());

    for (std::uint32_t id = 0; id < alive.size(); ++id)
    {
        /* Entity must be alive and fully renderable. */
        if (!alive[id])
        {
            continue;
        }

        if (!hasTransform[id] || !hasMesh[id] || !hasMaterial[id])
        {
            continue;
        }

        const TransformComponent& transform = transforms[id];
        const MeshComponent& mesh = meshes[id];
        const MaterialComponent& material = materials[id];

        RenderItem item{};
        item.MeshPtr = mesh.MeshPtr;
        item.MaterialPtr = material.MaterialPtr;
        item.Model = BuildModelMatrix(transform);

        outItems.push_back(item);
    }
}

/* Ensure internal arrays can hold entity id. */
void Scene::EnsureSize(std::uint32_t id)
{
    /* If we already have storage for this id, do nothing. */
    if (id < alive.size())
    {
        return;
    }

    /* We store components densely by entity id. */
    const std::size_t newSize = static_cast<std::size_t>(id) + 1;

    alive.resize(newSize, false);

    transforms.resize(newSize);
    meshes.resize(newSize);
    materials.resize(newSize);

    hasTransform.resize(newSize, false);
    hasMesh.resize(newSize, false);
    hasMaterial.resize(newSize, false);
}
