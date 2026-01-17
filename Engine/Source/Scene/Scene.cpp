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
#include <utility>

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

/* Move scene state. */
Scene::Scene(Scene&& other) noexcept
    : alive(std::move(other.alive))
    , componentStores(std::move(other.componentStores))
    , transformSystem(std::move(other.transformSystem))
{
    /* Transfer ownership of scene data. */
}

/* Move-assign scene state. */
Scene& Scene::operator=(Scene&& other) noexcept
{
    /* Guard against self-move. */
    if (this != &other)
    {
        alive = std::move(other.alive);
        componentStores = std::move(other.componentStores);
        transformSystem = std::move(other.transformSystem);
    }

    return *this;
}

/* Create a new entity and mark it alive. */
Entity Scene::CreateEntity()
{
    /* This uses a simple monotonic id allocation. */
    const std::uint32_t id = static_cast<std::uint32_t>(alive.size());

    /* Ensure storage for the new entity. */
    EnsureSize(id);

    /* Mark the entity alive. */
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

    /* Remove all components owned by this entity. */
    for (auto& entry : componentStores)
    {
        entry.second->RemoveForEntity(id);
    }

    /* Remove transform cache if present. */
    transformSystem.RemoveTransform(id);
}

/* Retrieve transform component if present. */
TransformComponent* Scene::GetTransform(Entity entity)
{
    return GetComponent<TransformComponent>(entity);
}

/* Retrieve mesh component if present. */
MeshComponent* Scene::GetMesh(Entity entity)
{
    return GetComponent<MeshComponent>(entity);
}

/* Retrieve material component if present. */
MaterialComponent* Scene::GetMaterial(Entity entity)
{
    return GetComponent<MaterialComponent>(entity);
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
    return AddComponent<TransformComponent>(entity);
}

/* Attach mesh component to entity. */
MeshComponent& Scene::AddMesh(Entity entity)
{
    return AddComponent<MeshComponent>(entity);
}

/* Attach material component to entity. */
MaterialComponent& Scene::AddMaterial(Entity entity)
{
    return AddComponent<MaterialComponent>(entity);
}

/* Build renderable items from active scene entities. */
void Scene::BuildRenderList(std::vector<RenderItem>& outItems) const
{
    /* Caller gets a clean list every time. */
    outItems.clear();

    /* Resolve component storage once per frame. */
    const ComponentStorage<TransformComponent>* transformStorage =
        FindStorage<TransformComponent>();
    const ComponentStorage<MeshComponent>* meshStorage =
        FindStorage<MeshComponent>();
    const ComponentStorage<MaterialComponent>* materialStorage =
        FindStorage<MaterialComponent>();

    if (!transformStorage || !meshStorage || !materialStorage)
    {
        return;
    }

    /* Avoid repeated reallocations when many entities are alive. */
    outItems.reserve(alive.size());

    for (std::uint32_t id = 0; id < alive.size(); ++id)
    {
        /* Entity must be alive and fully renderable. */
        if (!alive[id])
        {
            continue;
        }

        const TransformComponent* transform = transformStorage->Get(id);
        if (!transform)
        {
            continue;
        }

        const MeshComponent* mesh = meshStorage->Get(id);
        if (!mesh)
        {
            continue;
        }

        const MaterialComponent* material = materialStorage->Get(id);
        if (!material)
        {
            continue;
        }

        RenderItem item{};
        item.MeshPtr = mesh->MeshPtr;
        item.MaterialPtr = material->MaterialPtr;
        item.Model = transformSystem.GetModelMatrix(id, *transform);

        outItems.push_back(item);
    }
}

/* Enumerate living entities in the scene. */
void Scene::GetEntities(std::vector<Entity>& outEntities) const
{
    outEntities.clear();
    outEntities.reserve(alive.size());

    for (std::uint32_t id = 0; id < alive.size(); ++id)
    {
        if (alive[id])
        {
            outEntities.emplace_back(id);
        }
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

    /* Resize id-indexed state. */
    const std::size_t newSize = static_cast<std::size_t>(id) + 1;

    alive.resize(newSize, false);
}
