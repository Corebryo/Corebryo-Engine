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

#include "VulkanSurface.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdio>

/* Initialize empty surface state. */
VulkanSurface::VulkanSurface()
    : Surface(VK_NULL_HANDLE)
    , InstanceHandle(VK_NULL_HANDLE)
{
}

/* Destroy surface if still owned. */
VulkanSurface::~VulkanSurface()
{
    if (Surface != VK_NULL_HANDLE &&
        InstanceHandle != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(InstanceHandle, Surface, nullptr);
        Surface = VK_NULL_HANDLE;
        InstanceHandle = VK_NULL_HANDLE;
    }
}

/* Create Vulkan surface from window handle. */
bool VulkanSurface::Create(
    VkInstance Instance,
    GLFWwindow* WindowHandle)
{
    /* Store owning instance. */
    InstanceHandle = Instance;

    /* Create window surface. */
    const VkResult result =
        glfwCreateWindowSurface(
            Instance, WindowHandle, nullptr, &Surface);

    if (result != VK_SUCCESS)
    {
        std::fprintf(
            stderr,
            "VulkanSurface::Create: failed to create window surface (%d)\n",
            static_cast<int>(result));

        Surface = VK_NULL_HANDLE;
        InstanceHandle = VK_NULL_HANDLE;
        return false;
    }

    return true;
}

/* Destroy Vulkan surface. */
void VulkanSurface::Destroy(VkInstance Instance)
{
    /* Select active instance. */
    const VkInstance activeInstance =
        (Instance != VK_NULL_HANDLE)
        ? Instance
        : InstanceHandle;

    if (Surface != VK_NULL_HANDLE &&
        activeInstance != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(activeInstance, Surface, nullptr);
    }

    Surface = VK_NULL_HANDLE;
    InstanceHandle = VK_NULL_HANDLE;
}

/* Get surface handle. */
VkSurfaceKHR VulkanSurface::GetHandle() const
{
    return Surface;
}
