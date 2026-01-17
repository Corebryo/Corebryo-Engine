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

#define _CRT_SECURE_NO_WARNINGS
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_STANDARD_ASSERT
#define NK_IMPLEMENTATION
#define NK_GLFW_VULKAN_IMPLEMENTATION

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4305 4805 4244)
#endif

#include "nuklear.h"
#include "Renderer/Vulkan/Render/Nuklear/NuklearGlfwVulkan.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "NuklearOverlay.h"

#include <cstdio>

NuklearOverlay::NuklearOverlay()
    : Context(nullptr)
    , WindowHandle(nullptr)
    , MaxVertexBuffer(512 * 1024)
    , MaxIndexBuffer(128 * 1024)
    , Initialized(false)
    , LastDeltaTime(0.0f)
    , LastFps(0.0f)
    , LastDrawCalls(0)
    , LastTriangleCount(0)
    , LastVertexCount(0)
    , SelectedEntity()
    , Inspector()
    , PendingTransformEdit()
{
}

NuklearOverlay::~NuklearOverlay()
{
}

bool NuklearOverlay::Initialize(
    GLFWwindow* windowHandle,
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VkFormat colorFormat,
    const std::vector<VkImageView>& imageViews,
    VkExtent2D extent)
{
    if (!windowHandle || device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE)
    {
        return false;
    }

    WindowHandle = windowHandle;
    SwapchainImageViews = imageViews;

    Context = nk_glfw3_init(
        windowHandle,
        device,
        physicalDevice,
        graphicsQueueFamily,
        SwapchainImageViews.data(),
        static_cast<std::uint32_t>(SwapchainImageViews.size()),
        colorFormat,
        NK_GLFW3_DEFAULT,
        MaxVertexBuffer,
        MaxIndexBuffer);

    if (!Context)
    {
        return false;
    }

    (void)extent;
    UploadFonts(graphicsQueue);
    Initialized = true;
    return true;
}

void NuklearOverlay::Shutdown()
{
    if (!Initialized)
    {
        return;
    }

    nk_glfw3_shutdown();
    Context = nullptr;
    WindowHandle = nullptr;
    SwapchainImageViews.clear();
    Initialized = false;
}

void NuklearOverlay::BeginFrame(float deltaTime)
{
    if (!Initialized || !Context)
    {
        return;
    }

    LastDeltaTime = deltaTime;
    LastFps = deltaTime > 0.0001f ? 1.0f / deltaTime : 0.0f;

    nk_glfw3_new_frame();

    const struct nk_rect bounds = nk_rect(12.0f, 12.0f, 220.0f, 110.0f);
    const nk_flags flags = NK_WINDOW_NO_INPUT | NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER;

    if (nk_begin(Context, "Performance", bounds, flags))
    {
        nk_layout_row_dynamic(Context, 18.0f, 1);
        nk_labelf(Context, NK_TEXT_LEFT, "FPS: %.1f", LastFps);
        nk_labelf(Context, NK_TEXT_LEFT, "Frame: %.2f ms", LastDeltaTime * 1000.0f);
        nk_labelf(Context, NK_TEXT_LEFT, "Draw Calls: %u", LastDrawCalls);
        nk_labelf(Context, NK_TEXT_LEFT, "Triangles: %llu", static_cast<unsigned long long>(LastTriangleCount));
        nk_labelf(Context, NK_TEXT_LEFT, "Vertices: %llu", static_cast<unsigned long long>(LastVertexCount));
    }

    nk_end(Context);

    const struct nk_rect entityBounds = nk_rect(12.0f, 130.0f, 220.0f, 260.0f);
    const nk_flags entityFlags = NK_WINDOW_BORDER | NK_WINDOW_TITLE;

    if (nk_begin(Context, "Entities", entityBounds, entityFlags))
    {
        nk_layout_row_dynamic(Context, 18.0f, 1);
        if (SceneEntities.empty())
        {
            nk_label(Context, "No entities", NK_TEXT_LEFT);
        }
        else
        {
            for (const Entity& entity : SceneEntities)
            {
                char label[32]{};
                std::snprintf(label, sizeof(label), "Entity %u", entity.GetId());

                bool isSelected = entity.GetId() == SelectedEntity.GetId();
                if (nk_selectable_label(Context, label, NK_TEXT_LEFT, &isSelected))
                {
                    if (isSelected)
                    {
                        SelectedEntity = entity;
                    }
                    else
                    {
                        SelectedEntity = Entity();
                    }
                }
            }
        }
    }

    nk_end(Context);

    const struct nk_rect inspectorBounds = nk_rect(244.0f, 12.0f, 260.0f, 320.0f);
    const nk_flags inspectorFlags = NK_WINDOW_BORDER | NK_WINDOW_TITLE;

    if (nk_begin(Context, "Inspector", inspectorBounds, inspectorFlags))
    {
        nk_layout_row_dynamic(Context, 18.0f, 1);

        if (!Inspector.HasSelection)
        {
            nk_label(Context, "No entity selected", NK_TEXT_LEFT);
        }
        else
        {
            nk_labelf(Context, NK_TEXT_LEFT, "Entity %u", Inspector.SelectedEntity.GetId());

            nk_layout_row_dynamic(Context, 18.0f, 1);
            nk_label(Context, "Transform", NK_TEXT_LEFT);

            if (Inspector.HasTransform)
            {
                float position[3] = { Inspector.Position[0], Inspector.Position[1], Inspector.Position[2] };
                float rotation[3] = { Inspector.Rotation[0], Inspector.Rotation[1], Inspector.Rotation[2] };
                float scale[3] = { Inspector.Scale[0], Inspector.Scale[1], Inspector.Scale[2] };

                nk_layout_row_dynamic(Context, 18.0f, 1);
                nk_property_float(Context, "Pos X", -1000.0f, &position[0], 1000.0f, 0.1f, 0.01f);
                nk_property_float(Context, "Pos Y", -1000.0f, &position[1], 1000.0f, 0.1f, 0.01f);
                nk_property_float(Context, "Pos Z", -1000.0f, &position[2], 1000.0f, 0.1f, 0.01f);
                nk_property_float(Context, "Rot X", -360.0f, &rotation[0], 360.0f, 0.5f, 0.05f);
                nk_property_float(Context, "Rot Y", -360.0f, &rotation[1], 360.0f, 0.5f, 0.05f);
                nk_property_float(Context, "Rot Z", -360.0f, &rotation[2], 360.0f, 0.5f, 0.05f);
                nk_property_float(Context, "Scale X", 0.001f, &scale[0], 1000.0f, 0.1f, 0.01f);
                nk_property_float(Context, "Scale Y", 0.001f, &scale[1], 1000.0f, 0.1f, 0.01f);
                nk_property_float(Context, "Scale Z", 0.001f, &scale[2], 1000.0f, 0.1f, 0.01f);

                bool changed = false;
                for (int i = 0; i < 3; ++i)
                {
                    if (position[i] != Inspector.Position[i] ||
                        rotation[i] != Inspector.Rotation[i] ||
                        scale[i] != Inspector.Scale[i])
                    {
                        changed = true;
                        break;
                    }
                }

                if (changed)
                {
                    PendingTransformEdit.HasEdit = true;
                    PendingTransformEdit.Target = Inspector.SelectedEntity;
                    for (int i = 0; i < 3; ++i)
                    {
                        PendingTransformEdit.Position[i] = position[i];
                        PendingTransformEdit.Rotation[i] = rotation[i];
                        PendingTransformEdit.Scale[i] = scale[i];
                        Inspector.Position[i] = position[i];
                        Inspector.Rotation[i] = rotation[i];
                        Inspector.Scale[i] = scale[i];
                    }
                }
            }
            else
            {
                nk_label(Context, "No TransformComponent", NK_TEXT_LEFT);
            }

            nk_layout_row_dynamic(Context, 18.0f, 1);
            nk_label(Context, "Components", NK_TEXT_LEFT);
            nk_labelf(Context, NK_TEXT_LEFT, "MeshComponent: %s", Inspector.HasMesh ? "Yes" : "No");
            nk_labelf(Context, NK_TEXT_LEFT, "MaterialComponent: %s", Inspector.HasMaterial ? "Yes" : "No");
        }
    }

    nk_end(Context);
}

void NuklearOverlay::SetSceneEntities(const std::vector<Entity>& entities)
{
    SceneEntities = entities;
}

void NuklearOverlay::SetSelectedEntity(Entity entity)
{
    SelectedEntity = entity;
}

Entity NuklearOverlay::GetSelectedEntity() const
{
    return SelectedEntity;
}

void NuklearOverlay::SetInspectorData(const InspectorData& data)
{
    if (Inspector.HasSelection != data.HasSelection ||
        Inspector.SelectedEntity.GetId() != data.SelectedEntity.GetId())
    {
        PendingTransformEdit.HasEdit = false;
    }

    Inspector = data;
}

bool NuklearOverlay::ConsumeTransformEdit(TransformEdit& outEdit)
{
    if (!PendingTransformEdit.HasEdit)
    {
        return false;
    }

    outEdit = PendingTransformEdit;
    PendingTransformEdit.HasEdit = false;
    return true;
}

void NuklearOverlay::SetRenderStats(
    std::uint32_t drawCalls,
    std::uint64_t triangleCount,
    std::uint64_t vertexCount)
{
    LastDrawCalls = drawCalls;
    LastTriangleCount = triangleCount;
    LastVertexCount = vertexCount;
}

VkSemaphore NuklearOverlay::Render(
    VkQueue graphicsQueue,
    std::uint32_t imageIndex,
    VkSemaphore waitSemaphore)
{
    if (!Initialized)
    {
        return waitSemaphore;
    }

    return nk_glfw3_render(
        graphicsQueue,
        imageIndex,
        waitSemaphore,
        NK_ANTI_ALIASING_ON);
}

void NuklearOverlay::Resize(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    std::uint32_t graphicsQueueFamily,
    VkQueue graphicsQueue,
    VkFormat colorFormat,
    const std::vector<VkImageView>& imageViews,
    VkExtent2D extent)
{
    if (!Initialized)
    {
        return;
    }

    SwapchainImageViews = imageViews;

    nk_glfw3_device_destroy();
    nk_glfw3_device_create(
        device,
        physicalDevice,
        graphicsQueueFamily,
        SwapchainImageViews.data(),
        static_cast<std::uint32_t>(SwapchainImageViews.size()),
        colorFormat,
        MaxVertexBuffer,
        MaxIndexBuffer,
        extent.width,
        extent.height);

    UploadFonts(graphicsQueue);
}

bool NuklearOverlay::IsInitialized() const
{
    return Initialized;
}

void NuklearOverlay::UploadFonts(VkQueue graphicsQueue)
{
    struct nk_font_atlas* atlas = nullptr;
    nk_glfw3_font_stash_begin(&atlas);
    nk_glfw3_font_stash_end(graphicsQueue);
}
