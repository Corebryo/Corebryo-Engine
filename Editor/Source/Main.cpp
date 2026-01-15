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

#include "Platform/Input/GlfwInput.h"
#include "Platform/Window/GlfwWindow.h"
#include "Renderer/Vulkan/Core/VulkanDevice.h"
#include "Renderer/Vulkan/Core/VulkanInstance.h"
#include "Renderer/Vulkan/Render/VulkanRenderer.h"
#include "Renderer/Vulkan/Render/VulkanRenderPass.h"
#include "Renderer/Vulkan/Swapchain/VulkanSurface.h"
#include "Renderer/Vulkan/Swapchain/VulkanSwapchain.h"
#include "Engine/EngineState.h"
#include "Input/InputState.h"
#include "Scene/Scene.h"
#include "Scene/Collision/AABB.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdint>
#include <vector>

void SetHighPerformancePowerMode();

/* Simple scope-exit helper for reliable cleanup paths. */
template <typename TFunc>
class ScopeExit
{
public:
    /* Store cleanup callback and arm the scope guard. */
    explicit ScopeExit(TFunc func)
        : Func(func)
        , Active(true)
    {
        /* Default state is active. */
    }

    /* Run the cleanup callback if the guard is still active. */
    ~ScopeExit()
    {
        /* Only execute when not released. */
        if (Active)
        {
            /* Execute deferred cleanup. */
            Func();
        }
    }

    /* Disarm the guard so the destructor does nothing. */
    void Release()
    {
        /* Disable deferred cleanup. */
        Active = false;
    }

private:
    /* Stored cleanup function invoked on scope exit. */
    TFunc Func;

    /* Tracks whether the cleanup function should run. */
    bool Active;
};

/* Clamp helper to keep delta time stable when the app stalls. */
static float ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue)
    {
        return minValue;
    }

    if (value > maxValue)
    {
        return maxValue;
    }

    return value;
}

/* Check if a point lies inside an axis-aligned bounding box. */
static bool IsPointInsideAABB(const Vec3& point, const AABB& bounds)
{
    return point.x >= bounds.Min.x && point.x <= bounds.Max.x &&
        point.y >= bounds.Min.y && point.y <= bounds.Max.y &&
        point.z >= bounds.Min.z && point.z <= bounds.Max.z;
}

int main()
{
    /* Basic engine configuration for the bootstrap app. */
    constexpr std::uint32_t DefaultWindowWidth = 1280;
    constexpr std::uint32_t DefaultWindowHeight = 720;
    constexpr bool EnableCube = true;
    constexpr float CubeForwardOffset = 6.0f;
    constexpr float CameraCollisionRadius = 0.25f;

    /* Delta time configuration used by camera update. */
    constexpr float MinDeltaTime = 0.0f;
    constexpr float MaxDeltaTime = 0.05f;

    std::printf("Starting engine initialization...\n");

    SetHighPerformancePowerMode();

    /* Create the native window first because Vulkan needs a surface provider. */
    GlfwWindow window;
    bool windowCreated = false;

    if (!window.Create(DefaultWindowWidth, DefaultWindowHeight, "Engine"))
    {
        std::fprintf(stderr, "Failed to create window\n");
        return 1;
    }

    windowCreated = true;

    /* Ensure input is connected to the active GLFW window. */
    GlfwInput::Attach(window.GetHandle());

    /* Bring window to front early, but keep it hidden until the first frame is ready. */
    window.BringToFront();

    /* Use the real framebuffer size for initial swapchain creation. */
    std::int32_t windowWidth = 0;
    std::int32_t windowHeight = 0;
    window.GetSize(windowWidth, windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0)
    {
        windowWidth = static_cast<std::int32_t>(DefaultWindowWidth);
        windowHeight = static_cast<std::int32_t>(DefaultWindowHeight);
    }

    /* Create Vulkan instance with required extensions from GLFW window. */
    VulkanInstance instance;
    bool instanceCreated = false;

    if (!instance.Create("Engine", window.GetHandle()))
    {
        std::fprintf(stderr, "Failed to create Vulkan instance\n");

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    instanceCreated = true;

    /* Create the Vulkan device and query queue family details. */
    VulkanDevice device;
    bool deviceCreated = false;

    if (!device.Create(instance.GetHandle()))
    {
        std::fprintf(stderr, "Failed to create Vulkan device\n");

        if (instanceCreated)
        {
            instance.Destroy();
        }

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    deviceCreated = true;

    /* Create a surface used by the swapchain for presentation. */
    VulkanSurface surface;
    bool surfaceCreated = false;

    if (!surface.Create(instance.GetHandle(), window.GetHandle()))
    {
        std::fprintf(stderr, "Failed to create Vulkan surface\n");

        if (deviceCreated)
        {
            device.Destroy();
        }

        if (instanceCreated)
        {
            instance.Destroy();
        }

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    surfaceCreated = true;

    /* Create the swapchain with the initial window dimensions. */
    VulkanSwapchain swapchain;
    bool swapchainCreated = false;

    if (!swapchain.Create(
        device.GetPhysicalDevice(),
        device.GetDevice(),
        surface.GetHandle(),
        device.GetGraphicsQueueFamily(),
        static_cast<std::uint32_t>(windowWidth),
        static_cast<std::uint32_t>(windowHeight)))
    {
        std::fprintf(stderr, "Failed to create Vulkan swapchain\n");

        if (surfaceCreated)
        {
            surface.Destroy(instance.GetHandle());
        }

        if (deviceCreated)
        {
            device.Destroy();
        }

        if (instanceCreated)
        {
            instance.Destroy();
        }

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    swapchainCreated = true;

    /* Create a render pass matching the swapchain format. */
    VulkanRenderPass renderPass;
    bool renderPassCreated = false;

    if (!renderPass.Create(device.GetDevice(), swapchain.GetImageFormat()))
    {
        std::fprintf(stderr, "Failed to create Vulkan render pass\n");

        if (swapchainCreated)
        {
            swapchain.Destroy(device.GetDevice());
        }

        if (surfaceCreated)
        {
            surface.Destroy(instance.GetHandle());
        }

        if (deviceCreated)
        {
            device.Destroy();
        }

        if (instanceCreated)
        {
            instance.Destroy();
        }

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    renderPassCreated = true;

    /* Create the renderer that owns framebuffers and rendering pipelines. */
    VulkanRenderer renderer;
    bool rendererCreated = false;

    if (!renderer.Create(
        device.GetDevice(),
        device.GetPhysicalDevice(),
        device.GetGraphicsQueueFamily(),
        device.GetGraphicsQueue(),
        renderPass.GetHandle(),
        swapchain.GetImageFormat(),
        swapchain.GetImageViews(),
        swapchain.GetExtent(),
        swapchain.GetHandle()))
    {
        std::fprintf(stderr, "Failed to create Vulkan renderer\n");

        if (renderPassCreated)
        {
            renderPass.Destroy(device.GetDevice());
        }

        if (swapchainCreated)
        {
            swapchain.Destroy(device.GetDevice());
        }

        if (surfaceCreated)
        {
            surface.Destroy(instance.GetHandle());
        }

        if (deviceCreated)
        {
            device.Destroy();
        }

        if (instanceCreated)
        {
            instance.Destroy();
        }

        if (windowCreated)
        {
            window.Destroy();
        }

        return 1;
    }

    rendererCreated = true;

    /* Ensure we always shut down cleanly, even if we add new early returns later. */
    ScopeExit cleanup([&]()
        {
            if (deviceCreated)
            {
                vkDeviceWaitIdle(device.GetDevice());
            }

            if (rendererCreated)
            {
                renderer.Destroy(device.GetDevice());
            }

            if (renderPassCreated)
            {
                renderPass.Destroy(device.GetDevice());
            }

            if (swapchainCreated)
            {
                swapchain.Destroy(device.GetDevice());
            }

            if (deviceCreated)
            {
                device.Destroy();
            }

            if (surfaceCreated)
            {
                surface.Destroy(instance.GetHandle());
            }

            if (instanceCreated)
            {
                instance.Destroy();
            }

            if (windowCreated)
            {
                window.Destroy();
            }
        });

    /* Create a simple scene and optionally spawn a test entity. */
    Scene worldScene;
    const Vec3 cubePosition =
        renderer.GetCameraPosition() + Vec3(0.0f, 0.0f, -CubeForwardOffset);

    if (EnableCube)
    {
        const Entity cubeEntity = worldScene.CreateEntity();

        TransformComponent& transform = worldScene.AddTransform(cubeEntity);
        transform.Position = cubePosition;

        MeshComponent& mesh = worldScene.AddMesh(cubeEntity);
        mesh.MeshPtr = renderer.GetCubeMesh();

        MaterialComponent& material = worldScene.AddMaterial(cubeEntity);
        material.MaterialPtr = renderer.GetCubeMaterial();
    }

    /* Render list used to feed the renderer each frame. */
    std::vector<RenderItem> renderItems;
    renderItems.reserve(1024);

    /* Build the first frame before showing the window to avoid a blank flash. */
    worldScene.BuildRenderList(renderItems);
    renderer.SetRenderItems(renderItems);
    renderer.DrawFrame(device.GetDevice(), device.GetGraphicsQueue());

    glfwShowWindow(window.GetHandle());

    /* Time tracking based on GLFW monotonic timer. */
    double lastTime = glfwGetTime();

    /* Main loop runs until the window requests close. */
    while (!window.ShouldClose())
    {
        /* Start a new input frame. */
        InputBeginFrame();

        /* Poll input and window events. */
        window.PollEvents();

        /* Sync engine and input modes before applying cursor state. */
        SetInputMode(
            g_CurrentEngineState == EngineState::Game
            ? InputMode::Game
            : InputMode::Editor);

        /* Apply engine input mode and mouse capture state. */
        GlfwInput::ApplyInputMode(window.GetHandle());

        /* Snapshot input for deterministic updates. */
        InputEndFrame();

        /* Compute delta time and clamp it for stability. */
        const double nowTime = glfwGetTime();
        const float deltaTime = ClampFloat(static_cast<float>(nowTime - lastTime), MinDeltaTime, MaxDeltaTime);
        lastTime = nowTime;

        /* Cache the previous camera position for collision response. */
        const Vec3 previousCameraPosition = renderer.GetCameraPosition();

        /* Update the camera controller with the real delta time. */
        renderer.UpdateCamera(deltaTime);

        /* Prevent entering the cube while in the game state. */
        if (EnableCube && g_CurrentEngineState == EngineState::Game)
        {
            AABB cubeBounds{};
            cubeBounds.Min = Vec3(-0.5f, -0.5f, -0.5f) + cubePosition;
            cubeBounds.Max = Vec3(0.5f, 0.5f, 0.5f) + cubePosition;

            const Vec3 padding(
                CameraCollisionRadius,
                CameraCollisionRadius,
                CameraCollisionRadius);
            AABB paddedBounds{};
            paddedBounds.Min = cubeBounds.Min - padding;
            paddedBounds.Max = cubeBounds.Max + padding;

            if (IsPointInsideAABB(renderer.GetCameraPosition(), paddedBounds))
            {
                renderer.SetCameraPosition(previousCameraPosition);
            }
        }

        /* Handle window resize by recreating swapchain dependent resources. */
        if (window.WasResized())
        {
            std::int32_t width = 0;
            std::int32_t height = 0;
            window.GetSize(width, height);

            if (width > 0 && height > 0)
            {
                vkDeviceWaitIdle(device.GetDevice());

                swapchain.Recreate(
                    device.GetPhysicalDevice(),
                    device.GetDevice(),
                    surface.GetHandle(),
                    device.GetGraphicsQueueFamily(),
                    static_cast<std::uint32_t>(width),
                    static_cast<std::uint32_t>(height));

                renderer.Recreate(
                    device.GetDevice(),
                    device.GetGraphicsQueueFamily(),
                    device.GetGraphicsQueue(),
                    renderPass.GetHandle(),
                    swapchain.GetImageFormat(),
                    swapchain.GetImageViews(),
                    swapchain.GetExtent(),
                    swapchain.GetHandle());
            }

            window.ResetResizeFlag();
        }

        /* Skip rendering when the swapchain is effectively minimized. */
        if (swapchain.GetExtent().width == 0 || swapchain.GetExtent().height == 0)
        {
            continue;
        }

        /* Build and submit render items for this frame. */
        worldScene.BuildRenderList(renderItems);
        renderer.SetRenderItems(renderItems);
        renderer.DrawFrame(device.GetDevice(), device.GetGraphicsQueue());
    }

    /* Cleanup is performed by the scope guard. */
    cleanup.Release();

    return 0;
}
