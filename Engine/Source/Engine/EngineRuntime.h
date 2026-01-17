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

#include "Math/MathVector.h"
#include "Renderer/RenderItem.h"
#include "Renderer/Vulkan/Core/VulkanDevice.h"
#include "Renderer/Vulkan/Core/VulkanInstance.h"
#include "Renderer/Vulkan/Render/VulkanRenderer.h"
#include "Renderer/Vulkan/Render/VulkanRenderPass.h"
#include "Renderer/Vulkan/Swapchain/VulkanSurface.h"
#include "Renderer/Vulkan/Swapchain/VulkanSwapchain.h"
#include "Scene/Scene.h"

#include <cstdint>
#include <vector>

struct GLFWwindow;

/* Hosts the engine runtime for an external application (e.g., the editor). */
class EngineRuntime
{
public:
    /* Initialize engine runtime state. */
    EngineRuntime();

    /* Ensure the runtime is shut down. */
    ~EngineRuntime();

    /* Initialize the engine using a host-provided window. */
    bool Initialize(GLFWwindow* windowHandle);

    /* Update and render a single frame. */
    void Tick(float deltaTime);

    /* Shutdown all runtime systems. */
    void Shutdown();

    /* Handle host-driven window resize. */
    void OnResize(std::int32_t width, std::int32_t height);

    /* Query whether the runtime is initialized. */
    bool IsInitialized() const;

private:
    /* Create Vulkan and render resources. */
    bool CreateVulkanResources();

    /* Destroy Vulkan and render resources. */
    void DestroyVulkanResources();

    /* Build the initial scene. */
    void CreateScene();

    /* Draw the first frame to avoid blank flashes. */
    void BuildFirstFrame();

    GLFWwindow* WindowHandle;

    VulkanInstance Instance;
    VulkanDevice Device;
    VulkanSurface Surface;
    VulkanSwapchain Swapchain;
    VulkanRenderPass RenderPass;
    VulkanRenderer Renderer;

    Scene WorldScene;
    std::vector<RenderItem> RenderItems;
    std::vector<Entity> SceneEntities;
    Entity SelectedEntity;

    Vec3 CubePosition;

    bool InstanceCreated;
    bool DeviceCreated;
    bool SurfaceCreated;
    bool SwapchainCreated;
    bool RenderPassCreated;
    bool RendererCreated;
    bool Initialized;
};
