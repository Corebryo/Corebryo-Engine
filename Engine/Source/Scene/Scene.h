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

#include "Entity.h"
#include "ComponentStorage.h"
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "Components/MaterialComponent.h"
#include "TransformSystem.h"

#include "Renderer/RenderItem.h"

#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <unordered_map>

 /* Scene owns entity lifetime and component storage. */
class Scene
{
public:
    Scene();
    ~Scene();
    Scene(const Scene& other) = delete;
    Scene& operator=(const Scene& other) = delete;
    Scene(Scene&& other) noexcept;
    Scene& operator=(Scene&& other) noexcept;

    /* Entity lifecycle. */
    Entity CreateEntity();
    void DestroyEntity(Entity entity);

    /* Component accessors. */
    /* Caller must mark transforms dirty after mutation. */
    TransformComponent* GetTransform(Entity entity);
    MeshComponent* GetMesh(Entity entity);
    MaterialComponent* GetMaterial(Entity entity);

    /* Mark transform data dirty after modification. */
    void MarkTransformDirty(Entity entity);

    /* Component creation. */
    TransformComponent& AddTransform(Entity entity);
    MeshComponent& AddMesh(Entity entity);
    MaterialComponent& AddMaterial(Entity entity);

    /* Generic component accessors. */
    template <typename T>
    T& AddComponent(Entity entity);

    template <typename T>
    void RemoveComponent(Entity entity);

    template <typename T>
    bool HasComponent(Entity entity) const;

    template <typename T>
    T* GetComponent(Entity entity);

    template <typename T>
    const T* GetComponent(Entity entity) const;

    /* Build render submission list. */
    void BuildRenderList(std::vector<RenderItem>& outItems) const;

    /* Enumerate living entities. */
    void GetEntities(std::vector<Entity>& outEntities) const;

private:
    /* Ensure internal storage can hold entity id. */
    void EnsureSize(std::uint32_t id);

private:
    /* Resolve a stable component type id. */
    template <typename T>
    static std::size_t GetComponentTypeId();

    /* Find or create storage for a component type. */
    template <typename T>
    ComponentStorage<T>& GetOrCreateStorage();

    /* Find storage for a component type. */
    template <typename T>
    ComponentStorage<T>* FindStorage();

    /* Find storage for a component type. */
    template <typename T>
    const ComponentStorage<T>* FindStorage() const;

    /* Entity state. */
    std::vector<std::uint8_t> alive;

    /* Component storage. */
    std::unordered_map<std::size_t, std::unique_ptr<IComponentStorage>> componentStores;
    TransformSystem transformSystem;

};

template <typename T>
std::size_t Scene::GetComponentTypeId()
{
    /* Use a unique address as a stable type id. */
    static const char tag = 0;
    return reinterpret_cast<std::size_t>(&tag);
}

template <typename T>
ComponentStorage<T>& Scene::GetOrCreateStorage()
{
    const std::size_t typeId = GetComponentTypeId<T>();
    auto it = componentStores.find(typeId);
    if (it == componentStores.end())
    {
        /* Create storage for the component type. */
        auto storage = std::make_unique<ComponentStorage<T>>();
        ComponentStorage<T>* storagePtr = storage.get();
        componentStores.emplace(typeId, std::move(storage));
        return *storagePtr;
    }

    /* Return the existing typed storage. */
    return *static_cast<ComponentStorage<T>*>(it->second.get());
}

template <typename T>
ComponentStorage<T>* Scene::FindStorage()
{
    const std::size_t typeId = GetComponentTypeId<T>();
    auto it = componentStores.find(typeId);
    if (it == componentStores.end())
    {
        return nullptr;
    }

    /* Return the existing typed storage. */
    return static_cast<ComponentStorage<T>*>(it->second.get());
}

template <typename T>
const ComponentStorage<T>* Scene::FindStorage() const
{
    const std::size_t typeId = GetComponentTypeId<T>();
    auto it = componentStores.find(typeId);
    if (it == componentStores.end())
    {
        return nullptr;
    }

    /* Return the existing typed storage. */
    return static_cast<const ComponentStorage<T>*>(it->second.get());
}

template <typename T>
T& Scene::AddComponent(Entity entity)
{
    const std::uint32_t id = entity.GetId();

    /* Ensure entity storage exists. */
    EnsureSize(id);

    /* Resolve component storage for this type. */
    ComponentStorage<T>& storage = GetOrCreateStorage<T>();
    storage.EnsureSize(id);

    /* Add or reuse the component. */
    T& component = storage.Add(id);

    /* Create or update transform cache when applicable. */
    if constexpr (std::is_same_v<T, TransformComponent>)
    {
        transformSystem.AddTransform(id);
        transformSystem.MarkDirty(id);
    }

    return component;
}

template <typename T>
void Scene::RemoveComponent(Entity entity)
{
    const std::uint32_t id = entity.GetId();
    ComponentStorage<T>* storage = FindStorage<T>();
    if (!storage)
    {
        return;
    }

    /* Remove the component data. */
    storage->Remove(id);

    /* Remove transform cache when applicable. */
    if constexpr (std::is_same_v<T, TransformComponent>)
    {
        transformSystem.RemoveTransform(id);
    }
}

template <typename T>
bool Scene::HasComponent(Entity entity) const
{
    const std::uint32_t id = entity.GetId();
    const ComponentStorage<T>* storage = FindStorage<T>();
    if (!storage)
    {
        return false;
    }

    /* Query for component presence. */
    return storage->Has(id);
}

template <typename T>
T* Scene::GetComponent(Entity entity)
{
    const std::uint32_t id = entity.GetId();
    ComponentStorage<T>* storage = FindStorage<T>();
    if (!storage)
    {
        return nullptr;
    }

    /* Return the component address. */
    return storage->Get(id);
}

template <typename T>
const T* Scene::GetComponent(Entity entity) const
{
    const std::uint32_t id = entity.GetId();
    const ComponentStorage<T>* storage = FindStorage<T>();
    if (!storage)
    {
        return nullptr;
    }

    /* Return the component address. */
    return storage->Get(id);
}
