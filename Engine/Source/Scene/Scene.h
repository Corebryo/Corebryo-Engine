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
#include "TransformSystem.h"

#include "Renderer/RenderItem.h"

#include <vector>
#include <cstdint>
#include <limits>

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

    /* Build render submission list. */
    void BuildRenderList(std::vector<RenderItem>& outItems) const;

private:
    /* Ensure internal storage can hold entity id. */
    void EnsureSize(std::uint32_t id);

    /* Resolve mesh index for entity id. */
    std::uint32_t GetMeshIndex(std::uint32_t id) const;

    /* Resolve material index for entity id. */
    std::uint32_t GetMaterialIndex(std::uint32_t id) const;

    /* Remove mesh component for entity id. */
    void RemoveMeshComponent(std::uint32_t id);

    /* Remove material component for entity id. */
    void RemoveMaterialComponent(std::uint32_t id);

private:
    /* Invalid component index sentinel. */
    static constexpr std::uint32_t kInvalidComponentIndex =
        std::numeric_limits<std::uint32_t>::max();

    /* Entity state. */
    std::vector<std::uint8_t> alive;

    /* Component storage. */
    TransformSystem transformSystem;
    std::vector<MeshComponent> meshes;
    std::vector<std::uint32_t> meshEntities;
    std::vector<std::uint32_t> meshIndexByEntity;
    std::vector<MaterialComponent> materials;
    std::vector<std::uint32_t> materialEntities;
    std::vector<std::uint32_t> materialIndexByEntity;

};
