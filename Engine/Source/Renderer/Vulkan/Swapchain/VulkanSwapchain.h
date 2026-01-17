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

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

 /* Swapchain wrapper owning images and views. */
class VulkanSwapchain
{
public:
    /* Initialize empty swapchain state. */
    VulkanSwapchain();

    /* Destroy swapchain resources. */
    ~VulkanSwapchain();

    /* Create swapchain and image views. */
    bool Create(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkSurfaceKHR Surface,
        uint32_t GraphicsQueueFamily,
        uint32_t Width,
        uint32_t Height,
        bool EnableVsync);

    /* Recreate swapchain for resized surface. */
    bool Recreate(
        VkPhysicalDevice PhysicalDevice,
        VkDevice Device,
        VkSurfaceKHR Surface,
        uint32_t GraphicsQueueFamily,
        uint32_t Width,
        uint32_t Height,
        bool EnableVsync);

    /* Release swapchain and image views. */
    void Destroy(VkDevice Device);

    /* Get Vulkan swapchain handle. */
    VkSwapchainKHR GetHandle() const;

    /* Get swapchain image format. */
    VkFormat GetImageFormat() const;

    /* Get swapchain extent. */
    VkExtent2D GetExtent() const;

    /* Get image view handles. */
    std::vector<VkImageView> GetImageViews() const;

private:
    /* Vulkan swapchain handle. */
    VkSwapchainKHR Swapchain = VK_NULL_HANDLE;

    /* Swapchain image format. */
    VkFormat ImageFormat = VK_FORMAT_UNDEFINED;

    /* Swapchain resolution. */
    VkExtent2D SwapchainExtent{};

    /* Image views for swapchain images. */
    std::vector<VkImageView> SwapchainImageViews;
};
