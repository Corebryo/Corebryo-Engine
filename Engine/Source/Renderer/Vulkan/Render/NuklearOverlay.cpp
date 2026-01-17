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
