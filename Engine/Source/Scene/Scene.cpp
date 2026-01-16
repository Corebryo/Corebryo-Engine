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

#include <cstdint>

 /* Local helpers. */
namespace
{
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

    return Entity(id);
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
    transformSystem.RemoveComponent(id);
    RemoveMeshComponent(id);
    RemoveMaterialComponent(id);
}

/* Retrieve transform component if present. */
TransformComponent* Scene::GetTransform(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    return transformSystem.GetComponent(id);
}

/* Retrieve mesh component if present. */
MeshComponent* Scene::GetMesh(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Resolve packed mesh index. */
    const std::uint32_t index = GetMeshIndex(id);

    if (index == kInvalidComponentIndex)
    {
        return nullptr;
    }

    return &meshes[index];
}

/* Retrieve material component if present. */
MaterialComponent* Scene::GetMaterial(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Resolve packed material index. */
    const std::uint32_t index = GetMaterialIndex(id);

    if (index == kInvalidComponentIndex)
    {
        return nullptr;
    }

    return &materials[index];
}

/* Mark transform data dirty after modification. */
void Scene::MarkTransformDirty(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Mark transform cache dirty for this entity. */
    transformSystem.MarkDirty(id);
}

/* Attach transform component to entity. */
TransformComponent& Scene::AddTransform(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Add transform through the transform system. */
    return transformSystem.AddComponent(id);
}

/* Attach mesh component to entity. */
MeshComponent& Scene::AddMesh(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Reuse existing component if already present. */
    const std::uint32_t existingIndex = GetMeshIndex(id);
    if (existingIndex != kInvalidComponentIndex)
    {
        return meshes[existingIndex];
    }

    /* Append a packed mesh component. */
    const std::uint32_t newIndex = static_cast<std::uint32_t>(meshes.size());
    meshes.push_back(MeshComponent());
    meshEntities.push_back(id);
    meshIndexByEntity[id] = newIndex;

    return meshes[newIndex];
}

/* Attach material component to entity. */
MaterialComponent& Scene::AddMaterial(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure arrays exist for this id. */
    EnsureSize(id);

    /* Reuse existing component if already present. */
    const std::uint32_t existingIndex = GetMaterialIndex(id);
    if (existingIndex != kInvalidComponentIndex)
    {
        return materials[existingIndex];
    }

    /* Append a packed material component. */
    const std::uint32_t newIndex = static_cast<std::uint32_t>(materials.size());
    materials.push_back(MaterialComponent());
    materialEntities.push_back(id);
    materialIndexByEntity[id] = newIndex;

    return materials[newIndex];
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

        if (!transformSystem.HasComponent(id))
        {
            continue;
        }

        const std::uint32_t meshIndex = GetMeshIndex(id);
        if (meshIndex == kInvalidComponentIndex)
        {
            continue;
        }

        const std::uint32_t materialIndex = GetMaterialIndex(id);
        if (materialIndex == kInvalidComponentIndex)
        {
            continue;
        }

        const MeshComponent& mesh = meshes[meshIndex];
        const MaterialComponent& material = materials[materialIndex];

        RenderItem item{};
        item.MeshPtr = mesh.MeshPtr;
        item.MaterialPtr = material.MaterialPtr;
        item.Model = transformSystem.GetModelMatrix(id);

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

    /* Resize id-indexed state and sparse component mappings. */
    const std::size_t newSize = static_cast<std::size_t>(id) + 1;

    alive.resize(newSize, false);

    meshIndexByEntity.resize(newSize, kInvalidComponentIndex);
    materialIndexByEntity.resize(newSize, kInvalidComponentIndex);

    transformSystem.EnsureSize(id);
}

/* Resolve mesh index for entity id. */
std::uint32_t Scene::GetMeshIndex(std::uint32_t id) const
{
    if (!IsValidId(id, meshIndexByEntity.size()))
    {
        return kInvalidComponentIndex;
    }

    return meshIndexByEntity[id];
}

/* Resolve material index for entity id. */
std::uint32_t Scene::GetMaterialIndex(std::uint32_t id) const
{
    if (!IsValidId(id, materialIndexByEntity.size()))
    {
        return kInvalidComponentIndex;
    }

    return materialIndexByEntity[id];
}

/* Remove mesh component for entity id. */
void Scene::RemoveMeshComponent(std::uint32_t id)
{
    const std::uint32_t index = GetMeshIndex(id);
    if (index == kInvalidComponentIndex)
    {
        return;
    }

    const std::uint32_t lastIndex =
        static_cast<std::uint32_t>(meshes.size() - 1);

    if (index != lastIndex)
    {
        meshes[index] = meshes[lastIndex];
        meshEntities[index] = meshEntities[lastIndex];
        meshIndexByEntity[meshEntities[index]] = index;
    }

    /* Remove the last packed entry. */
    meshes.pop_back();
    meshEntities.pop_back();
    meshIndexByEntity[id] = kInvalidComponentIndex;
}

/* Remove material component for entity id. */
void Scene::RemoveMaterialComponent(std::uint32_t id)
{
    const std::uint32_t index = GetMaterialIndex(id);
    if (index == kInvalidComponentIndex)
    {
        return;
    }

    const std::uint32_t lastIndex =
        static_cast<std::uint32_t>(materials.size() - 1);

    if (index != lastIndex)
    {
        materials[index] = materials[lastIndex];
        materialEntities[index] = materialEntities[lastIndex];
        materialIndexByEntity[materialEntities[index]] = index;
    }

    /* Remove the last packed entry. */
    materials.pop_back();
    materialEntities.pop_back();
    materialIndexByEntity[id] = kInvalidComponentIndex;
}
