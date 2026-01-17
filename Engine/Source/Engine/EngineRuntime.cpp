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

#include "EngineRuntime.h"

#include "Engine/EngineState.h"
#include "Scene/Collision/AABB.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdint>

namespace
{
    /* Simple scene configuration. */
    constexpr bool kEnableCube = false;
    constexpr float kCubeForwardOffset = 6.0f;
    constexpr float kCameraCollisionRadius = 0.25f;

    /* Delta time clamp values. */
    constexpr float kMinDeltaTime = 0.0f;
    constexpr float kMaxDeltaTime = 0.05f;

    /* Clamp helper to keep delta time stable when the app stalls. */
    float ClampFloat(float value, float minValue, float maxValue)
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
    bool IsPointInsideAABB(const Vec3& point, const AABB& bounds)
    {
        return point.x >= bounds.Min.x && point.x <= bounds.Max.x &&
            point.y >= bounds.Min.y && point.y <= bounds.Max.y &&
            point.z >= bounds.Min.z && point.z <= bounds.Max.z;
    }
}

/* Initialize runtime defaults. */
EngineRuntime::EngineRuntime()
    : WindowHandle(nullptr)
    , Config()
    , InstanceCreated(false)
    , DeviceCreated(false)
    , SurfaceCreated(false)
    , SwapchainCreated(false)
    , RenderPassCreated(false)
    , RendererCreated(false)
    , Initialized(false)
{
    /* Defer initialization until Initialize is called. */
}

/* Ensure cleanup happens if the host forgets to shut down. */
EngineRuntime::~EngineRuntime()
{
    Shutdown();
}

/* Initialize the engine using a host-provided window. */
bool EngineRuntime::Initialize(GLFWwindow* windowHandle, const EngineConfig& config)
{
    Shutdown();

    if (!windowHandle)
    {
        std::fprintf(stderr, "EngineRuntime::Initialize: missing window handle\n");
        return false;
    }

    WindowHandle = windowHandle;
    Config = config;
    g_CurrentEngineState = Config.InitialState;

    if (!CreateVulkanResources())
    {
        DestroyVulkanResources();
        return false;
    }

    CreateScene();
    BuildFirstFrame();

    Initialized = true;
    return true;
}

/* Update and render a single frame. */
void EngineRuntime::Tick(float deltaTime)
{
    if (!Initialized)
    {
        return;
    }

    TickSimulation(deltaTime);
    TickEditorSyncPreRender();
    const bool didRender = TickRender(deltaTime);
    if (didRender)
    {
        TickEditorSyncPostRender();
    }
}

/* Shutdown all runtime systems. */
void EngineRuntime::Shutdown()
{
    if (Initialized && DeviceCreated)
    {
        vkDeviceWaitIdle(Device.GetDevice());
    }

    DestroyVulkanResources();

    WindowHandle = nullptr;
    RenderItems.clear();
    SceneEntities.clear();
    WorldScene = Scene();
    SelectedEntity = Entity();
    InspectorState = InspectorData();
    Initialized = false;
}

/* Handle host-driven window resize. */
void EngineRuntime::OnResize(std::int32_t width, std::int32_t height)
{
    if (!Initialized || !SwapchainCreated || !RendererCreated)
    {
        return;
    }

    if (width <= 0 || height <= 0)
    {
        return;
    }

    vkDeviceWaitIdle(Device.GetDevice());

    Swapchain.Recreate(
        Device.GetPhysicalDevice(),
        Device.GetDevice(),
        Surface.GetHandle(),
        Device.GetGraphicsQueueFamily(),
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        Config.EnableVsync);

    Renderer.Recreate(
        Device.GetDevice(),
        Device.GetGraphicsQueueFamily(),
        Device.GetGraphicsQueue(),
        RenderPass.GetHandle(),
        Swapchain.GetImageFormat(),
        Swapchain.GetImageViews(),
        Swapchain.GetExtent(),
        Swapchain.GetHandle());
}

/* Query whether the runtime is initialized. */
bool EngineRuntime::IsInitialized() const
{
    return Initialized;
}

void EngineRuntime::TickSimulation(float deltaTime)
{
    const float clampedDeltaTime = ClampFloat(deltaTime, kMinDeltaTime, kMaxDeltaTime);

    /* Cache the previous camera position for collision response. */
    const Vec3 previousCameraPosition = Renderer.GetCameraPosition();

    /* Update the camera controller with the real delta time. */
    Renderer.UpdateCamera(clampedDeltaTime);

    /* Prevent entering the cube while in the game state. */
    if (kEnableCube && g_CurrentEngineState == EngineState::Game)
    {
        AABB cubeBounds{};
        cubeBounds.Min = Vec3(-0.5f, -0.5f, -0.5f) + CubePosition;
        cubeBounds.Max = Vec3(0.5f, 0.5f, 0.5f) + CubePosition;

        const Vec3 padding(
            kCameraCollisionRadius,
            kCameraCollisionRadius,
            kCameraCollisionRadius);
        AABB paddedBounds{};
        paddedBounds.Min = cubeBounds.Min - padding;
        paddedBounds.Max = cubeBounds.Max + padding;

        if (IsPointInsideAABB(Renderer.GetCameraPosition(), paddedBounds))
        {
            Renderer.SetCameraPosition(previousCameraPosition);
        }
    }

    /* Build render items for this frame. */
    WorldScene.BuildRenderList(RenderItems);
}

void EngineRuntime::TickEditorSyncPreRender()
{
    WorldScene.GetEntities(SceneEntities);

    if (SelectedEntity.IsValid())
    {
        bool selectionValid = false;
        for (const Entity& entity : SceneEntities)
        {
            if (entity.GetId() == SelectedEntity.GetId())
            {
                selectionValid = true;
                break;
            }
        }

        if (!selectionValid)
        {
            SelectedEntity = Entity();
        }
    }

    InspectorData inspector{};
    inspector.HasSelection = SelectedEntity.IsValid();
    inspector.SelectedEntity = SelectedEntity;
    if (inspector.HasSelection)
    {
        const TransformComponent* transform = WorldScene.GetTransform(SelectedEntity);
        inspector.HasTransform = transform != nullptr;
        if (transform)
        {
            inspector.Position[0] = transform->Position.x;
            inspector.Position[1] = transform->Position.y;
            inspector.Position[2] = transform->Position.z;
            inspector.Rotation[0] = transform->Rotation.x;
            inspector.Rotation[1] = transform->Rotation.y;
            inspector.Rotation[2] = transform->Rotation.z;
            inspector.Scale[0] = transform->Scale.x;
            inspector.Scale[1] = transform->Scale.y;
            inspector.Scale[2] = transform->Scale.z;

            const AABB bounds = transform->GetUnitCubeAABB();
            inspector.BoundsMin[0] = bounds.Min.x;
            inspector.BoundsMin[1] = bounds.Min.y;
            inspector.BoundsMin[2] = bounds.Min.z;
            inspector.BoundsMax[0] = bounds.Max.x;
            inspector.BoundsMax[1] = bounds.Max.y;
            inspector.BoundsMax[2] = bounds.Max.z;
        }

        inspector.HasMesh = WorldScene.GetMesh(SelectedEntity) != nullptr;
        inspector.HasMaterial = WorldScene.GetMaterial(SelectedEntity) != nullptr;
        inspector.ComponentCount = static_cast<std::uint32_t>(inspector.HasTransform) +
            static_cast<std::uint32_t>(inspector.HasMesh) +
            static_cast<std::uint32_t>(inspector.HasMaterial);
    }

    InspectorState = inspector;
    Renderer.SetEditorEntities(SceneEntities);
    Renderer.SetEditorSelection(SelectedEntity);
    Renderer.SetInspectorData(InspectorState);
}

void EngineRuntime::TickEditorSyncPostRender()
{
    SelectedEntity = Renderer.GetEditorSelection();

    TransformEdit edit{};
    if (Renderer.ConsumeTransformEdit(edit))
    {
        TransformComponent* transform = WorldScene.GetTransform(edit.Target);
        if (transform)
        {
            transform->Position = Vec3(edit.Position[0], edit.Position[1], edit.Position[2]);
            transform->Rotation = Vec3(edit.Rotation[0], edit.Rotation[1], edit.Rotation[2]);
            transform->Scale = Vec3(edit.Scale[0], edit.Scale[1], edit.Scale[2]);
            WorldScene.MarkTransformDirty(edit.Target);
        }
    }
}

bool EngineRuntime::TickRender(float deltaTime)
{
    Renderer.SetOverlayTiming(deltaTime);

    /* Skip rendering when the swapchain is effectively minimized. */
    if (Swapchain.GetExtent().width == 0 || Swapchain.GetExtent().height == 0)
    {
        return false;
    }

    Renderer.SetRenderItems(RenderItems);
    Renderer.DrawFrame(Device.GetDevice(), Device.GetGraphicsQueue());
    return true;
}

/* Create Vulkan and render resources. */
bool EngineRuntime::CreateVulkanResources()
{
    std::int32_t windowWidth = 0;
    std::int32_t windowHeight = 0;
    glfwGetFramebufferSize(WindowHandle, &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0)
    {
        windowWidth = static_cast<std::int32_t>(Config.FallbackWidth);
        windowHeight = static_cast<std::int32_t>(Config.FallbackHeight);
    }

    if (!Instance.Create("Editor", WindowHandle))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create instance\n");
        return false;
    }

    InstanceCreated = true;

    if (!Device.Create(Instance.GetHandle()))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create device\n");
        return false;
    }

    DeviceCreated = true;

    if (!Surface.Create(Instance.GetHandle(), WindowHandle))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create surface\n");
        return false;
    }

    SurfaceCreated = true;

    if (!Swapchain.Create(
        Device.GetPhysicalDevice(),
        Device.GetDevice(),
        Surface.GetHandle(),
        Device.GetGraphicsQueueFamily(),
        static_cast<std::uint32_t>(windowWidth),
        static_cast<std::uint32_t>(windowHeight),
        Config.EnableVsync))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create swapchain\n");
        return false;
    }

    SwapchainCreated = true;

    if (!RenderPass.Create(Device.GetDevice(), Swapchain.GetImageFormat()))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create render pass\n");
        return false;
    }

    RenderPassCreated = true;

    if (!Renderer.Create(
        Device.GetDevice(),
        Device.GetPhysicalDevice(),
        Device.GetGraphicsQueueFamily(),
        Device.GetGraphicsQueue(),
        RenderPass.GetHandle(),
        Swapchain.GetImageFormat(),
        Swapchain.GetImageViews(),
        Swapchain.GetExtent(),
        Swapchain.GetHandle()))
    {
        std::fprintf(stderr, "EngineRuntime::CreateVulkanResources: failed to create renderer\n");
        return false;
    }

    RendererCreated = true;
    Renderer.InitializeOverlay(WindowHandle);
    return true;
}

/* Destroy Vulkan and render resources. */
void EngineRuntime::DestroyVulkanResources()
{
    if (DeviceCreated)
    {
        /* Destroy renderer resources before tearing down the device. */
        Renderer.Destroy(Device.GetDevice());
        RendererCreated = false;
    }

    if (RenderPassCreated)
    {
        RenderPass.Destroy(Device.GetDevice());
        RenderPassCreated = false;
    }

    if (SwapchainCreated)
    {
        Swapchain.Destroy(Device.GetDevice());
        SwapchainCreated = false;
    }

    if (DeviceCreated)
    {
        Device.Destroy();
        DeviceCreated = false;
    }

    if (SurfaceCreated)
    {
        Surface.Destroy(Instance.GetHandle());
        SurfaceCreated = false;
    }

    if (InstanceCreated)
    {
        Instance.Destroy();
        InstanceCreated = false;
    }
}

/* Build the initial scene. */
void EngineRuntime::CreateScene()
{
    WorldScene = Scene();
    RenderItems.clear();
    RenderItems.reserve(1024);
    SceneEntities.clear();
    SelectedEntity = Entity();
    InspectorState = InspectorData();

    CubePosition =
        Renderer.GetCameraPosition() + Vec3(0.0f, 0.0f, -kCubeForwardOffset);

    if (kEnableCube)
    {
        const Entity cubeEntity = WorldScene.CreateEntity();

        TransformComponent& transform = WorldScene.AddTransform(cubeEntity);
        transform.Position = CubePosition;

        MeshComponent& mesh = WorldScene.AddMesh(cubeEntity);
        mesh.MeshPtr = Renderer.GetCubeMesh();

        MaterialComponent& material = WorldScene.AddMaterial(cubeEntity);
        material.MaterialPtr = Renderer.GetCubeMaterial();
    }
}

/* Draw the first frame to avoid blank flashes. */
void EngineRuntime::BuildFirstFrame()
{
    WorldScene.BuildRenderList(RenderItems);
    Renderer.SetRenderItems(RenderItems);
    WorldScene.GetEntities(SceneEntities);
    Renderer.SetEditorEntities(SceneEntities);
    Renderer.SetEditorSelection(SelectedEntity);
    Renderer.SetInspectorData(InspectorState);
    Renderer.DrawFrame(Device.GetDevice(), Device.GetGraphicsQueue());
}
