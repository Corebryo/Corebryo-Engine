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

#include "Scene/Entity.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

struct GLFWwindow;
struct nk_context;

struct InspectorData
{
    bool HasSelection = false;
    Entity SelectedEntity{};
    bool HasTransform = false;
    float Position[3] = { 0.0f, 0.0f, 0.0f };
    float Rotation[3] = { 0.0f, 0.0f, 0.0f };
    float Scale[3] = { 1.0f, 1.0f, 1.0f };
    bool HasMesh = false;
    bool HasMaterial = false;
};

struct TransformEdit
{
    bool HasEdit = false;
    Entity Target{};
    float Position[3] = { 0.0f, 0.0f, 0.0f };
    float Rotation[3] = { 0.0f, 0.0f, 0.0f };
    float Scale[3] = { 1.0f, 1.0f, 1.0f };
};

/* Lightweight Nuklear overlay for editor performance stats. */
class NuklearOverlay
{
public:
    NuklearOverlay();
    ~NuklearOverlay();

    bool Initialize(
        GLFWwindow* windowHandle,
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        std::uint32_t graphicsQueueFamily,
        VkQueue graphicsQueue,
        VkFormat colorFormat,
        const std::vector<VkImageView>& imageViews,
        VkExtent2D extent);

    void Shutdown();

    void BeginFrame(float deltaTime);

    void SetSceneEntities(const std::vector<Entity>& entities);
    void SetSelectedEntity(Entity entity);
    Entity GetSelectedEntity() const;

    void SetInspectorData(const InspectorData& data);
    bool ConsumeTransformEdit(TransformEdit& outEdit);

    void SetRenderStats(
        std::uint32_t drawCalls,
        std::uint64_t triangleCount,
        std::uint64_t vertexCount);

    VkSemaphore Render(
        VkQueue graphicsQueue,
        std::uint32_t imageIndex,
        VkSemaphore waitSemaphore);

    void Resize(
        VkDevice device,
        VkPhysicalDevice physicalDevice,
        std::uint32_t graphicsQueueFamily,
        VkQueue graphicsQueue,
        VkFormat colorFormat,
        const std::vector<VkImageView>& imageViews,
        VkExtent2D extent);

    bool IsInitialized() const;

private:
    void UploadFonts(VkQueue graphicsQueue);

    nk_context* Context;
    GLFWwindow* WindowHandle;
    std::vector<VkImageView> SwapchainImageViews;
    VkDeviceSize MaxVertexBuffer;
    VkDeviceSize MaxIndexBuffer;
    bool Initialized;
    float LastDeltaTime;
    float LastFps;
    std::uint32_t LastDrawCalls;
    std::uint64_t LastTriangleCount;
    std::uint64_t LastVertexCount;
    std::vector<Entity> SceneEntities;
    Entity SelectedEntity;
    InspectorData Inspector;
    TransformEdit PendingTransformEdit;
};
