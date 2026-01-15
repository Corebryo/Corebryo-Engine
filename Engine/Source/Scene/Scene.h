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
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "Components/MaterialComponent.h"

#include "Renderer/RenderItem.h"

#include <vector>
#include <cstdint>

 /* Scene owns entity lifetime and component storage. */
class Scene
{
public:
    Scene();
    ~Scene();

    /* Entity lifecycle. */
    Entity CreateEntity();
    void DestroyEntity(Entity entity);

    /* Component accessors. */
    TransformComponent* GetTransform(Entity entity);
    MeshComponent* GetMesh(Entity entity);
    MaterialComponent* GetMaterial(Entity entity);

    /* Component creation. */
    TransformComponent& AddTransform(Entity entity);
    MeshComponent& AddMesh(Entity entity);
    MaterialComponent& AddMaterial(Entity entity);

    /* Build render submission list. */
    void BuildRenderList(std::vector<RenderItem>& outItems) const;

private:
    /* Ensure internal storage can hold entity id. */
    void EnsureSize(std::uint32_t id);

private:
    /* Entity state. */
    std::vector<std::uint8_t> alive;

    /* Component storage. */
    std::vector<TransformComponent> transforms;
    std::vector<MeshComponent> meshes;
    std::vector<MaterialComponent> materials;

    /* Component presence flags. */
    std::vector<std::uint8_t> hasTransform;
    std::vector<std::uint8_t> hasMesh;
    std::vector<std::uint8_t> hasMaterial;

};

/* Entity inline helpers */

inline Entity::Entity(std::uint32_t id, Scene* scene)
    : Id(id)
    , Owner(scene)
{
}

inline std::uint32_t Entity::GetId() const
{
    return Id;
}

inline bool Entity::IsValid() const
{
    return Owner != nullptr;
}

inline TransformComponent* Entity::GetTransform() const
{
    return Owner ? Owner->GetTransform(*this) : nullptr;
}

inline MeshComponent* Entity::GetMesh() const
{
    return Owner ? Owner->GetMesh(*this) : nullptr;
}

inline MaterialComponent* Entity::GetMaterial() const
{
    return Owner ? Owner->GetMaterial(*this) : nullptr;
}

inline TransformComponent& Entity::AddTransform()
{
    return Owner->AddTransform(*this);
}

inline MeshComponent& Entity::AddMesh()
{
    return Owner->AddMesh(*this);
}

inline MaterialComponent& Entity::AddMaterial()
{
    return Owner->AddMaterial(*this);
}
