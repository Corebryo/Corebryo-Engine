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

#include <cstddef>
#include <cstdint>
#include <vector>

/* Base interface for type-erased component storage. */
class IComponentStorage
{
public:
    virtual ~IComponentStorage() = default;

    /* Remove component data for an entity id. */
    virtual void RemoveForEntity(std::uint32_t id) = 0;

    /* Ensure sparse lookup can reference an entity id. */
    virtual void EnsureSize(std::uint32_t id) = 0;
};

/* Packed component storage with sparse lookup by entity id. */
template <typename T>
class ComponentStorage final : public IComponentStorage
{
public:
    ComponentStorage() = default;
    ~ComponentStorage() override = default;

    /* Get component pointer for entity id. */
    T* Get(std::uint32_t id)
    {
        /* Resolve packed index for the entity. */
        const std::uint32_t index = GetIndex(id);
        if (index == kInvalidIndex)
        {
            return nullptr;
        }

        /* Return the packed component address. */
        return &components[index];
    }

    /* Get component pointer for entity id. */
    const T* Get(std::uint32_t id) const
    {
        /* Resolve packed index for the entity. */
        const std::uint32_t index = GetIndex(id);
        if (index == kInvalidIndex)
        {
            return nullptr;
        }

        /* Return the packed component address. */
        return &components[index];
    }

    /* Add a component for entity id. */
    T& Add(std::uint32_t id)
    {
        /* Ensure the sparse lookup can reference the id. */
        EnsureSize(id);

        /* Return existing component if present. */
        const std::uint32_t existingIndex = GetIndex(id);
        if (existingIndex != kInvalidIndex)
        {
            return components[existingIndex];
        }

        /* Append a packed component slot. */
        const std::uint32_t newIndex =
            static_cast<std::uint32_t>(components.size());
        components.push_back(T());
        entityIds.push_back(id);
        indexByEntity[id] = newIndex;

        return components[newIndex];
    }

    /* Remove a component for entity id. */
    void Remove(std::uint32_t id)
    {
        /* Resolve the packed index for removal. */
        const std::uint32_t index = GetIndex(id);
        if (index == kInvalidIndex)
        {
            return;
        }

        /* Resolve the last packed index. */
        const std::uint32_t lastIndex =
            static_cast<std::uint32_t>(components.size() - 1);

        /* Swap the last entry into the removed slot. */
        if (index != lastIndex)
        {
            components[index] = components[lastIndex];
            entityIds[index] = entityIds[lastIndex];
            indexByEntity[entityIds[index]] = index;
        }

        /* Remove the last packed entry. */
        components.pop_back();
        entityIds.pop_back();
        indexByEntity[id] = kInvalidIndex;
    }

    /* Check whether an entity has this component. */
    bool Has(std::uint32_t id) const
    {
        /* Check for a valid packed index. */
        return GetIndex(id) != kInvalidIndex;
    }

    /* Remove component for entity id. */
    void RemoveForEntity(std::uint32_t id) override
    {
        /* Forward to the typed removal path. */
        Remove(id);
    }

    /* Ensure sparse lookup can reference an entity id. */
    void EnsureSize(std::uint32_t id) override
    {
        /* Early out when the lookup is large enough. */
        if (id < indexByEntity.size())
        {
            return;
        }

        /* Grow the sparse lookup table. */
        const std::size_t newSize = static_cast<std::size_t>(id) + 1;
        indexByEntity.resize(newSize, kInvalidIndex);
    }

private:
    /* Resolve packed index for an entity id. */
    std::uint32_t GetIndex(std::uint32_t id) const
    {
        /* Validate the sparse lookup range. */
        if (id >= indexByEntity.size())
        {
            return kInvalidIndex;
        }

        /* Return the packed index. */
        return indexByEntity[id];
    }

private:
    /* Invalid index sentinel. */
    static constexpr std::uint32_t kInvalidIndex = 0xFFFFFFFFu;

    /* Packed component data. */
    std::vector<T> components;

    /* Entity ids for packed components. */
    std::vector<std::uint32_t> entityIds;

    /* Sparse lookup table. */
    std::vector<std::uint32_t> indexByEntity;
};
